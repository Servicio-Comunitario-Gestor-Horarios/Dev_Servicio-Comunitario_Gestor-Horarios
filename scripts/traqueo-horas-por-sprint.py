#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Traqueo de horas por sprint — análisis read-only de actividad (git + GitHub + informes).

Genera docs/analisis-actividad/traqueo-horas-por-sprint-<cutoff>.md con:
tabla persona/sprint (días activos, horas/día en RANGO, commits, archivos creados,
PRs, issues), vista calendario, validación cruzada con informes institucionales,
apéndice metodológico y caveats. No modifica código, build ni CI.

Dependencias: python3 stdlib + git + gh (CLI). Reproducible: mismo estado del
repo + misma data de GitHub => mismo informe byte a byte (sin marca de tiempo).

Uso:
  python3 scripts/traqueo-horas-por-sprint.py
  python3 scripts/traqueo-horas-por-sprint.py --cutoff 2026-08-01 --gap-min 60
  python3 scripts/traqueo-horas-por-sprint.py --help
"""

import argparse
import datetime as dt
import json
import math
import os
import re
import subprocess
import sys
from concurrent.futures import ThreadPoolExecutor

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
GH_OWNER = "Servicio-Comunitario-Gestor-Horarios"

# gh username -> nombre canónico (verificado en el repo)
GH_USER_MAP = {
    "LuisRojas260305": "Luis Rojas",
    "IWinterI": "Daniel Reyna",
    "magrmanuel25": "Manuel García",
    "KPrusita": "Paola Peña",
    "Niko0510": "Nicole Sereno",
}

# fallback git email -> nombre canónico (si .mailmap no cubre alguna variante)
GIT_EMAIL_FALLBACK = {
    "luisalexanderrojasguevara@gmail.com": "Luis Rojas",
    "dan.a.reyna2018@gmail.com": "Daniel Reyna",
    "magrmanuel25@gmail.com": "Manuel García",
    "Paolalidhapp@gmail.com": "Paola Peña",
    "108741616+KPrusita@users.noreply.github.com": "Paola Peña",
    "serenonicole122@gmail.com": "Nicole Sereno",
}

EXPECTED_AUTHORS = {
    "Luis Rojas", "Daniel Reyna", "Manuel García", "Paola Peña", "Nicole Sereno",
}

# rutas excluidas del análisis (componentes de ruta: build*, .agents_backup)
EXCLUDED_COMPONENT_RE = re.compile(r"(?:^|/)(?:build[^/]*|\.agents_backup)(?:/|$)")
# binarios excluidos de "archivos creados" y del volumen de líneas
BINARY_RE = re.compile(
    r"\.(?:png|jpe?g|gif|bmp|ico|pdf|mp4|webm|avi|mov|zip|gz|7z|rar|exe|dll|so|dylib|a|o|obj|pyc|pyo|db|sqlite|sqlite3|xlsx|docx|pptx)$",
    re.I,
)

MESES = {
    "enero": 1, "febrero": 2, "marzo": 3, "abril": 4, "mayo": 5, "junio": 6,
    "julio": 7, "agosto": 8, "septiembre": 9, "octubre": 10, "noviembre": 11,
    "diciembre": 12,
}

HEADER_RE = re.compile(r"^([0-9a-f]+)\|(.+?)\|(.+)$")
WEEK_RE = re.compile(r"Semana del (\d+) de (\w+) al (\d+) de (\w+) de (\d{4})")
APORTES_RE = re.compile(
    r"^### ([^\n]+?) — [^\n]*\n\*(\d+) contribuci[oó]n(?:es)? esta semana\*", re.M
)

DAY_HEADERS = ["L", "M", "X", "J", "V", "S", "D"]

FLOOR_HOURS = 1.0
CAP_HOURS = 12.0

# anclas de timelapses locales (evidencia de sesiones de Luis)
TIMELAPSE_ANCHORS = [
    (dt.date(2026, 7, 5), "S2"),
    (dt.date(2026, 7, 14), "S4"),
    (dt.date(2026, 7, 20), "S5"),
    (dt.date(2026, 7, 28), "S6"),
]

INFO_DIR = os.path.join(ROOT, "docs", "informes")
TIMELAPSE_DIR = os.path.join(ROOT, "docs", "timelapses")


# ---------------------------------------------------------------- helpers

def run(cmd, cwd=ROOT, check=True, timeout=120):
    """Ejecuta un comando; devuelve stdout. check=True => lanza si exit != 0."""
    proc = subprocess.run(cmd, cwd=cwd, capture_output=True, text=True, timeout=timeout)
    if check and proc.returncode != 0:
        raise RuntimeError(
            "Comando falló (exit %d): %s\n%s" % (proc.returncode, " ".join(cmd), proc.stderr[:800])
        )
    return proc.stdout


def gh(args, check=True, timeout=120):
    """Ejecuta gh; devuelve el CompletedProcess (para .stdout/.stderr/.returncode)."""
    return subprocess.run(["gh"] + args, cwd=ROOT, capture_output=True, text=True,
                          timeout=timeout, check=check)


def detect_repo_tz():
    """Zona horaria del repo (fechas de commit), p.ej. -0400."""
    out = run(["git", "log", "-1", "--format=%ad", "--date=format:%z"]).strip()
    m = re.match(r"([+-])(\d{2})(\d{2})", out)
    sign = 1 if m.group(1) == "+" else -1
    minutes = sign * (int(m.group(2)) * 60 + int(m.group(3)))
    return dt.timezone(dt.timedelta(minutes=minutes))


def parse_ts(text):
    """'2026-07-05T14:17:28-04:00' (git) o '2026-07-05T18:17:28Z' (gh) -> datetime aware."""
    return dt.datetime.fromisoformat(text)


def sprint_of(d, sprints):
    for name, start, end in sprints:
        if start is None:
            if d <= end:
                return name
        elif start <= d <= end:
            return name
    return None


def build_sprints(cutoff):
    """Tabla canónica S1..S6; S6 termina en el cutoff."""
    return [
        ("S1", None, dt.date(2026, 6, 28)),
        ("S2", dt.date(2026, 6, 29), dt.date(2026, 7, 5)),
        ("S3", dt.date(2026, 7, 6), dt.date(2026, 7, 12)),
        ("S4", dt.date(2026, 7, 13), dt.date(2026, 7, 19)),
        ("S5", dt.date(2026, 7, 20), dt.date(2026, 7, 26)),
        ("S6", dt.date(2026, 7, 27), cutoff),
    ]


def canonicalize_author(name, email):
    if name in EXPECTED_AUTHORS:
        return name
    return GIT_EMAIL_FALLBACK.get(email, name)


def canonicalize_gh(login):
    return GH_USER_MAP.get(login, login)


def is_excluded_path(path):
    return bool(EXCLUDED_COMPONENT_RE.search(path))


def is_binary_path(path):
    return bool(BINARY_RE.search(path))


# ---------------------------------------------------------------- git

def stash_only_commits():
    """Commits alcanzables desde refs/stash pero desde ningún otro ref."""
    refs = [r.strip() for r in run(["git", "for-each-ref", "--format=%(refname)"]).splitlines() if r.strip()]
    if "refs/stash" not in refs:
        return set()
    others = [r for r in refs if r != "refs/stash"]
    out = run(["git", "rev-list", "refs/stash", "--not"] + others)
    return set(out.split())


def git_commits(stash_only, tz):
    """Commits (--all, mailmap). Excluye commits solo-alcanzables vía refs/stash."""
    fmt = "%H|%aN|%aE|%ad|%cd"
    out = run(["git", "log", "--all", "--use-mailmap",
               "--pretty=format:" + fmt, "--date=iso-strict"])
    commits = []
    for line in out.splitlines():
        parts = line.split("|")
        if len(parts) < 5:
            continue
        h, an, ae, ad = parts[0], parts[1], parts[2], parts[3]
        if h in stash_only:
            continue
        ts = parse_ts(ad)
        commits.append({
            "hash": h,
            "author": canonicalize_author(an, ae),
            "email": ae,
            "ts": ts,
            "day": ts.astimezone(tz).date(),
        })
    return commits


def git_creations(stash_only, tz):
    """Archivos creados (proxy = primer commit que añade el path, sobre --all)."""
    fmt = "%H|%aN|%ad"
    out = run(["git", "log", "--all", "--use-mailmap", "--diff-filter=A",
               "--pretty=format:" + fmt, "--date=iso-strict", "--name-only"])
    cur = None
    by_path = {}
    for line in out.splitlines():
        m = HEADER_RE.match(line)
        if m:
            cur = (m.group(1), m.group(2), parse_ts(m.group(3)))
            continue
        if line.strip() and cur is not None:
            h, an, ts = cur
            if h in stash_only:
                continue
            path = line.strip()
            if is_excluded_path(path) or is_binary_path(path):
                continue
            prev = by_path.get(path)
            if prev is None or ts < prev["ts"]:
                by_path[path] = {
                    "path": path,
                    "author": canonicalize_author(an, ""),
                    "ts": ts,
                    "day": ts.astimezone(tz).date(),
                }
    return list(by_path.values())


def git_numstat(stash_only, tz):
    """Por commit: rutas tocadas (excluir commits solo-build) y líneas +/- de texto."""
    fmt = "%H|%aN|%ad"
    out = run(["git", "log", "--all", "--use-mailmap", "--numstat",
               "--pretty=format:" + fmt, "--date=iso-strict"])
    cur = None
    commits = []
    for line in out.splitlines():
        m = HEADER_RE.match(line)
        if m:
            h, an, ad = m.group(1), m.group(2), m.group(3)
            if h in stash_only:
                cur = None
                continue
            ts = parse_ts(ad)
            cur = {
                "hash": h, "author": canonicalize_author(an, ""),
                "ts": ts, "day": ts.astimezone(tz).date(),
                "files": [], "add": 0, "del": 0,
            }
            commits.append(cur)
            continue
        if not line.strip():
            cur = None
            continue
        if cur is None:
            continue
        parts = line.split("\t", 2)
        if len(parts) < 3:
            continue
        add, dele, path = parts[0], parts[1], parts[2]
        cur["files"].append(path)
        if is_excluded_path(path) or is_binary_path(path):
            continue
        if add.isdigit() and dele.isdigit():
            cur["add"] += int(add)
            cur["del"] += int(dele)
    return commits


def shortlog_authors():
    out = run(["git", "shortlog", "-sne", "--all"])
    names = []
    for line in out.splitlines():
        if "\t" in line:
            names.append(line.split("\t", 1)[1].split("<")[0].strip())
    return names


# ---------------------------------------------------------------- github

def gh_prs():
    proc = gh(["pr", "list", "--state", "all", "--limit", "100",
               "--json", "number,state,author,createdAt,mergedAt,closedAt,title"])
    data = json.loads(proc.stdout)
    if len(data) >= 100:
        print("AVISO: gh pr list alcanzó el límite de 100 — posible truncado")
    return data


def gh_issues():
    proc = gh(["issue", "list", "--state", "all", "--limit", "100",
               "--json", "number,labels,assignees,createdAt,closedAt,title"])
    data = json.loads(proc.stdout)
    if len(data) >= 100:
        print("AVISO: gh issue list alcanzó el límite de 100 — posible truncado")
    # rango del diseño: issues #6–41 (los demás no tienen labels de sprint)
    return [i for i in data if 6 <= i["number"] <= 41]


def gh_pr_review_events(prs):
    """Reviews y comentarios de PR por número (evidencia de actividad de revisión)."""
    def fetch(pr):
        n = pr["number"]
        proc = gh(["pr", "view", str(n), "--json", "reviews,comments"], check=False, timeout=90)
        if proc.returncode != 0:
            return n, []
        try:
            data = json.loads(proc.stdout)
        except json.JSONDecodeError:
            return n, []
        events = []
        for r in data.get("reviews") or []:
            login = (r.get("author") or {}).get("login")
            ts = r.get("submittedAt")
            if login and ts:
                events.append((login, ts))
        for c in data.get("comments") or []:
            login = (c.get("author") or {}).get("login")
            ts = c.get("createdAt")
            if login and ts:
                events.append((login, ts))
        return n, events

    with ThreadPoolExecutor(max_workers=8) as ex:
        results = list(ex.map(fetch, prs))
    return dict(results)


def gh_projects_v2_note():
    """Projects v2: intento de lectura; sin scope read:project => aviso, no falla."""
    proc = gh(["project", "list", "--owner", GH_OWNER, "--format", "json"], check=False, timeout=90)
    if proc.returncode == 0:
        try:
            projects = json.loads(proc.stdout)
        except json.JSONDecodeError:
            projects = []
        if projects:
            titles = ", ".join(p.get("title", "?") for p in projects[:5])
            return ("ok", "Projects v2 leído: %d proyecto(s): %s. Timestamps de tarjetas "
                    "disponibles como evidencia de actividad (no integrados en el modelo de horas)."
                    % (len(projects), titles))
        return ("ok", "Projects v2 leído: sin proyectos.")
    err = (proc.stderr or "").strip()
    if "read:project" in err or "missing required scopes" in err.lower():
        return ("warn", "Projects v2 excluido — token sin scope read:project")
    return ("warn", "Projects v2 no disponible: %s" % err[:200])

# ---------------------------------------------------------------- eventos

def build_events(commits, prs, issues, review_events, tz, sprints):
    """Eventos normalizados {persona, dia, ts epoch, fuente, meta}."""
    events = []
    for c in commits:
        events.append({"persona": c["author"], "day": c["day"], "ts": c["ts"].timestamp(),
                       "src": "commit", "meta": {"hash": c["hash"]}})
    for p in prs:
        author = canonicalize_gh((p.get("author") or {}).get("login", ""))
        for key in ("createdAt", "mergedAt", "closedAt"):
            raw = p.get(key)
            if not raw:
                continue
            ts = parse_ts(raw)
            d = ts.astimezone(tz).date()
            if d <= sprints[-1][2]:
                events.append({"persona": author, "day": d, "ts": ts.timestamp(),
                               "src": "pr", "meta": {"pr": p["number"], "momento": key}})
    for i in issues:
        sprint_label = None
        for lab in i.get("labels") or []:
            name = lab.get("name", "")
            if name.startswith("sprint-"):
                sprint_label = "S" + name.split("-")[1]
        for a in i.get("assignees") or []:
            person = canonicalize_gh(a.get("login", ""))
            for key in ("createdAt", "closedAt"):
                raw = i.get(key)
                if not raw:
                    continue
                ts = parse_ts(raw)
                d = ts.astimezone(tz).date()
                if d <= sprints[-1][2]:
                    events.append({"persona": person, "day": d, "ts": ts.timestamp(),
                                   "src": "issue", "meta": {"issue": i["number"], "momento": key}})
    for n, evs in review_events.items():
        for login, raw in evs:
            ts = parse_ts(raw)
            d = ts.astimezone(tz).date()
            if d <= sprints[-1][2]:
                events.append({"persona": canonicalize_gh(login), "day": d, "ts": ts.timestamp(),
                               "src": "review", "meta": {"pr": n}})
    events.sort(key=lambda e: (e["persona"], e["day"], e["ts"], e["src"]))
    return events


def hours_for_day(ts_list, gap_minutes):
    """Horas de un día: sesiones separadas por gap > umbral; horas = span del día
    menos los gaps entre sesiones (piso 1h, techo 12h)."""
    ts = sorted(ts_list)
    if not ts:
        return FLOOR_HOURS
    sessions = [[ts[0]]]
    for a, b in zip(ts, ts[1:]):
        if (b - a) / 60.0 > gap_minutes:
            sessions.append([b])
        else:
            sessions[-1].append(b)
    minutes = sum(s[-1] - s[0] for s in sessions) / 60.0
    return max(FLOOR_HOURS, min(minutes / 60.0, CAP_HOURS))


# ---------------------------------------------------------------- agregación

def aggregate(events, sprints):
    """Días activos por persona+sprint (dedup persona+día, cualquier fuente)."""
    days = {}
    for e in events:
        sprint = sprint_of(e["day"], sprints)
        if sprint is None:
            continue
        days.setdefault(e["persona"], {}).setdefault(sprint, set()).add(e["day"])
    return {p: {s: len(ds) for s, ds in sprints_map.items()} for p, sprints_map in days.items()}


def compute_hours(events, sprints, gap_min):
    """Horas/día por persona+sprint: escenarios gap±15 (default 45/60/90) -> rango."""
    scenarios = [max(15, gap_min - 15), gap_min, gap_min + 15]
    by_person_day = {}
    for e in events:
        if e["src"] == "review":
            continue  # las revisiones no alimentan el modelo de horas
        sprint = sprint_of(e["day"], sprints)
        if sprint is None:
            continue
        by_person_day.setdefault(e["persona"], {}).setdefault((sprint, e["day"]), []).append(e["ts"])

    out = {}
    for person, sprint_days in by_person_day.items():
        for (sprint, day), tss in sprint_days.items():
            entry = out.setdefault(person, {}).setdefault(sprint, {"days": [], "scen": {}})
            entry["days"].append(day)
            for s in scenarios:
                entry["scen"].setdefault(s, []).append(hours_for_day(tss, s))
    for person in out:
        for sprint in out[person]:
            entry = out[person][sprint]
            for s in scenarios:
                vals = entry["scen"][s]
                entry["scen"][s] = sum(vals) / len(vals) if vals else 0.0
            entry["n"] = len(entry["days"])
    return out, scenarios


def compute_volume(commits, creations, prs, issues, sprints):
    """Volumen por persona+sprint: commits, archivos creados, PRs, issues, revisiones."""
    vol = {}

    def bump(person, sprint, key, n=1):
        if sprint is None:
            return
        vol.setdefault(person, {}).setdefault(sprint, {}).setdefault(key, 0)
        vol[person][sprint][key] += n

    for c in commits:
        bump(c["author"], sprint_of(c["day"], sprints), "commits")
    for f in creations:
        bump(f["author"], sprint_of(f["day"], sprints), "archivos")
    for p in prs:
        author = canonicalize_gh((p.get("author") or {}).get("login", ""))
        d = parse_ts(p["createdAt"]).astimezone(dt.timezone(dt.timedelta(hours=-4))).date()
        sprint = sprint_of(d, sprints)
        merged = bool(p.get("mergedAt"))
        bump(author, sprint, "prs_total")
        bump(author, sprint, "prs_merged" if merged else "prs_closed")
    for i in issues:
        sprint_label = None
        for lab in i.get("labels") or []:
            name = lab.get("name", "")
            if name.startswith("sprint-"):
                sprint_label = "S" + name.split("-")[1]
        created_day = parse_ts(i["createdAt"]).astimezone(dt.timezone(dt.timedelta(hours=-4))).date()
        sprint = sprint_label or sprint_of(created_day, sprints)
        assignees = [canonicalize_gh(a.get("login", "")) for a in (i.get("assignees") or [])]
        closed_raw = i.get("closedAt")
        for a in assignees:
            bump(a, sprint, "issues_asig")
            if closed_raw:
                closed_day = parse_ts(closed_raw).astimezone(dt.timezone(dt.timedelta(hours=-4))).date()
                cs = sprint_of(closed_day, sprints)
                bump(a, cs, "issues_res")
    return vol


def lines_volume(numstat, sprints):
    """Líneas de texto +/- por persona+sprint (apéndice)."""
    vol = {}
    for c in numstat:
        sprint = sprint_of(c["day"], sprints)
        if sprint is None:
            continue
        d = vol.setdefault(c["author"], {}).setdefault(sprint, {"add": 0, "del": 0})
        d["add"] += c["add"]
        d["del"] += c["del"]
    return vol


# ---------------------------------------------------------------- validación cruzada

def parse_informes(sprints):
    """Informes institucionales semana_{2..5} -> {sprint: {persona: contribuciones}}."""
    out = {}
    notas = []
    for week in (2, 3, 4, 5):
        path = os.path.join(INFO_DIR, "informe_sprint_institucional_semana_%d.md" % week)
        if not os.path.exists(path):
            notas.append("informe semana_%d no encontrado — validación omitida" % week)
            continue
        with open(path, encoding="utf-8") as fh:
            text = fh.read()
        sprint = None
        m = WEEK_RE.search(text)
        if m:
            d = dt.date(int(m.group(5)), MESES[m.group(2)], int(m.group(1)))
            sprint = sprint_of(d, sprints)
        if sprint is None:
            sprint = "S%d" % week
        contribs = {}
        for raw_name, n in APORTES_RE.findall(text):
            contribs[raw_name.split("—")[0].strip()] = int(n)
        out[sprint] = contribs
    return out, notas


def cross_validate(informes, days, notas):
    """Informe vs evidencia git/gh; discrepancias como notas, nunca ocultas."""
    for sprint in sorted(informes):
        for person in sorted(informes[sprint]):
            count = informes[sprint][person]
            git_days = days.get(person, {}).get(sprint, 0)
            if count == 0 and git_days > 0:
                notas.append(
                    "Discrepancia %s: el informe reporta '%s: 0 contribuciones' pero la "
                    "evidencia git/gh muestra %d día(s) activo(s)." % (sprint, person, git_days))
            elif count > 0 and git_days == 0:
                notas.append(
                    "Discrepancia %s: el informe reporta '%s: %d contribuciones' sin "
                    "evidencia git/gh (0 días activos)." % (sprint, person, count))


# ---------------------------------------------------------------- corroboración local

SKIP_DIRS = {".git", ".agents", ".claude", ".opencode", ".qtcreator", ".vendor", "Testing"}


def local_mtime_days(out_path, tz):
    """Días con mtime local de archivos del árbol (solo corroboración de sesiones locales)."""
    days = set()
    out_abs = os.path.abspath(out_path)
    for dirpath, dirnames, filenames in os.walk(ROOT):
        dirnames[:] = [d for d in dirnames
                       if d not in SKIP_DIRS and not d.startswith("build")]
        if os.path.abspath(dirpath).startswith(os.path.dirname(out_abs)):
            continue
        for fn in filenames:
            p = os.path.join(dirpath, fn)
            if os.path.abspath(p) == out_abs:
                continue
            try:
                m = os.path.getmtime(p)
            except OSError:
                continue
            day = dt.datetime.fromtimestamp(m, tz).date()
            if day <= dt.date(2026, 8, 1):
                days.add(day)
    return sorted(days)


def timelapse_anchors():
    """Anclas de timelapses (archivos locales): esperada actividad de Luis cerca."""
    info = []
    for d, sprint in TIMELAPSE_ANCHORS:
        path = os.path.join(TIMELAPSE_DIR, "timelapse-%s.mp4" % d.isoformat())
        info.append((d, sprint, os.path.exists(path)))
    return info

# ---------------------------------------------------------------- render

def fmt_span(d):
    return "–" if d is None else d.isoformat()


def fmt_hours_range(lo, hi):
    if hi - lo < 0.05:
        return "%dh" % round(lo)
    return "%d–%dh" % (math.floor(lo), math.ceil(hi))


def render_report(out_path, args, tz, sprints, days, hours, hours_scen, volume,
                  lines_vol, informes, notas, review_tot, projects_note, local_days,
                  tl_info, merged_n, closed_n, total_commits, authors, closed_prs,
                  stash_hashes, cutoff):
    L = []
    add = L.append
    add("> ADVERTENCIA: ESTIMACIÓN — no horas reales. Modelo span−gaps (umbral 60 min, "
        "piso 1h, techo 12h), incertidumbre ±30–50%%. Corte %s." % cutoff.isoformat())
    add("> Los valores de horas son RANGOS estimados con escenarios de 45/60/90 min "
        "(conservador/medio/optimista). Prohibido citarlos como horas reales.")
    add("> Generado por `scripts/traqueo-horas-por-sprint.py` — reproducible: mismo estado "
        "del repo + misma data de GitHub => mismo informe byte a byte.")
    add("")
    add("# Traqueo de horas por sprint — %s" % cutoff.isoformat())
    add("")
    add("Informe read-only de actividad del equipo (git `--all` + GitHub PRs/issues + "
        "informes institucionales). Estimación de días activos y horas/día en RANGO por "
        "persona y sprint, con vistas calendario, validación cruzada y apéndice metodológico.")
    add("")

    # ---- tabla canónica
    add("## Tabla canónica de sprints")
    add("")
    add("| Sprint | Inicio | Fin |")
    add("|---|---|---|")
    for name, start, end in sprints:
        add("| %s | %s | %s |" % (name, fmt_span(start), fmt_span(end)))
    add("")
    add("Nota: S1 = fase de bootstrap previa a la planificación (commits del 05-10 al 06-28). "
        "S6 termina en el corte del análisis (%s)." % cutoff.isoformat())
    add("")

    # ---- resumen
    add("## Resumen por persona/sprint")
    add("")
    add("Columnas: días activos (cualquier fuente: commit, PR, issue, revisión); horas/día "
        "RANGO (solo fuentes de horas: commit, PR, issue); commits; archivos creados "
        "(primer commit que añade el archivo); PRs (crédito completo, `Nm/Nc` = merged / "
        "cerrado-sin-merge); issues (asignados/resueltos).")
    add("")
    add("| Persona | Sprint | Días activos | Horas/día (rango) | Commits | Arch. creados | PRs | Issues |")
    add("|---|---|---|---|---|---|---|---|")
    personas = sorted({p for p in days} | {p for p in hours} | {p for p in volume})
    for person in personas:
        for sprint_name, _, _ in sprints:
            nd = days.get(person, {}).get(sprint_name, 0)
            h = hours.get(person, {}).get(sprint_name)
            v = volume.get(person, {}).get(sprint_name, {})
            if nd == 0:
                add("| %s | %s | — | datos insuficientes | — | — | — | — |" % (person, sprint_name))
                continue
            if h is None:
                hcell = "solo revisiones*"
            else:
                lo = min(h["scen"].values())
                hi = max(h["scen"].values())
                unc = int(round((hi - lo) / hi * 100)) if hi > 0 else 0
                hcell = "%s (±%d%%)" % (fmt_hours_range(lo, hi), unc)
                if unc > 50:
                    hcell += " **"
            prs = v.get("prs_total", 0)
            prm = v.get("prs_merged", 0)
            prc = v.get("prs_closed", 0)
            prcell = str(prs) if prs else "—"
            if prc:
                prcell = "%d (%dm/%dc)" % (prs, prm, prc)
            issues_cell = "%d/%d" % (v.get("issues_asig", 0), v.get("issues_res", 0)) \
                if v.get("issues_asig") or v.get("issues_res") else "—"
            add("| %s | %s | %d | %s | %d | %d | %s | %s |" % (
                person, sprint_name, nd, hcell,
                v.get("commits", 0), v.get("archivos", 0), prcell, issues_cell))
    add("")
    add("* = día(s) con actividad solo de revisión (no alimenta el modelo de horas; se "
        "aplica el piso de 1h).")
    add("** = las variantes de escenario difieren >50%: ver apéndice B.")
    add("")

    # ---- calendario
    add("## Calendario — qué días trabajó")
    add("")
    add("`X` = actividad (commit/PR/issue), `R` = solo revisión, `–` = sin actividad. "
        "Fechas en tz del repo (%s)." % tz)
    add("")
    add("| Persona | Sprint | L | M | X | J | V | S | D |")
    add("|---|---|---|---|---|---|---|---|---|")
    for person in personas:
        for sprint_name, start, end in sprints:
            if start is None:
                start = end - dt.timedelta(days=6)
            week = [start + dt.timedelta(days=i) for i in range(7)]
            cells = []
            for d in week:
                evs = [e for e in all_events if e["persona"] == person and e["day"] == d]
                if not evs:
                    cells.append("–")
                elif all(e["src"] == "review" for e in evs):
                    cells.append("R")
                else:
                    cells.append("X")
            add("| %s | %s | %s |" % (person, sprint_name, " | ".join(cells)))
    add("")
    add("Nota: el calendario de S1 muestra la semana previa al corte de S1 (22–28 jun); "
        "actividad anterior (bootstrap 05-10/05-12, solo Luis Rojas) queda fuera de la rejilla.")
    add("")

    # ---- revisión
    add("## Actividad de revisión de PRs")
    add("")
    add("Comentarios y reviews de PR cuentan como evidencia de días activos del revisor, "
        "pero NO alimentan el modelo de horas (fuente demasiado dispersa).")
    add("")
    add("| Persona | Sprint | Eventos de revisión |")
    add("|---|---|---|")
    for person in sorted(review_tot):
        for sprint_name in [s[0] for s in sprints]:
            n = review_tot[person].get(sprint_name, 0)
            if n:
                add("| %s | %s | %d |" % (person, sprint_name, n))
    add("")

    # ---- validación cruzada
    add("## Validación cruzada con informes institucionales")
    add("")
    add("Comparación de las contribuciones declaradas en "
        "`docs/informes/informe_sprint_institucional_semana_{2..5}.md` contra los días "
        "activos evidenciados en git/GitHub (mismo sprint por fechas).")
    add("")
    add("| Sprint | Persona | Informe (contribuciones) | Evidencia (días activos) |")
    add("|---|---|---|---|")
    for sprint_name in sorted(informes):
        for person in sorted(informes[sprint_name]):
            c = informes[sprint_name][person]
            g = days.get(person, {}).get(sprint_name, 0)
            add("| %s | %s | %d | %d |" % (sprint_name, person, c, g))
    add("")
    add("Las discrepancias se documentan en la sección Notas; nunca se ocultan.")
    add("")

    # ---- corroboración local
    add("## Corroboración local (mtime) y timelapses")
    add("")
    add("El mtime de archivos en el árbol de trabajo solo corrobora sesiones locales en "
        "esta máquina (Luis Rojas); nunca se atribuye a otros miembros y no alimenta el "
        "modelo de horas.")
    add("")
    add("Días con actividad local de archivos (mtime, tz %s): %s" % (
        tz, ", ".join(d.isoformat() for d in local_days) if local_days else "ninguno"))
    add("")
    add("Anclas de timelapses:")
    for d, sprint, exists in tl_info:
        add("- `docs/timelapses/timelapse-%s.mp4` (%s, %s): %s" % (
            d.isoformat(), sprint, "existe" if exists else "no existe",
            "sesión local esperada de Luis" if exists else "sin ancla local"))
    add("")

    # ---- projects v2
    add("## Projects v2")
    add("")
    add("Intento de lectura del tablero de la org (%s): %s" % (GH_OWNER, projects_note[1]))
    add("")

    # ---- notas
    add("## Notas y discrepancias")
    add("")
    if notas:
        for n in notas:
            add("- %s" % n)
    else:
        add("- Sin notas.")
    if closed_prs:
        add("- PRs cerrados sin merge (cuentan como trabajo completo, marcados en la "
            "columna PRs): %s." % ", ".join("#%d" % p["number"] for p in closed_prs))
    if stash_hashes:
        add("- %d commits alcanzables solo vía `refs/stash` (WIP/index no mergeados) "
            "excluidos de la actividad: %s." % (len(stash_hashes), ", ".join(stash_hashes)))
    add("")

    # ---- apéndice
    add("## Apéndice A — Metodología")
    add("")
    add("**Modelo de horas (por persona y día):**")
    add("1. Eventos de horas = commits (fecha de autor), eventos de PR (creación, merge o "
        "cierre) y eventos de issue (creación, cierre), atribuidos a la persona "
        "correspondiente. Los eventos de revisión NO alimentan el modelo.")
    add("2. Para cada día: span = primer a último evento; las sesiones se dividen cuando el "
        "gap entre eventos supera el umbral (default 60 min).")
    add("3. Horas/día = span del día menos los gaps entre sesiones (= suma de duraciones de "
        "sesiones). Piso 1h, techo 12h.")
    add("4. Escenarios de umbral: conservador (45 min), medio (60 min), optimista (90 min). "
        "El rango reportado = [mín, máx] entre escenarios; incertidumbre = (máx−mín)/máx, "
        "típicamente 30–50%.")
    add("5. Si las variantes difieren >50%, el detalle por escenario se muestra abajo "
        "(apéndice B).")
    add("")
    add("**Fuentes y zonas horarias:** git (`--all`, `.mailmap`) usa la tz del repo "
        "(%s); GitHub devuelve timestamps en UTC (Z). Todo se normaliza a epoch y se "
        "agrupa por día en la tz del repo." % tz)
    add("")
    add("**Atribución:** `.mailmap` canonicaliza las 8 variantes de autor a 5 nombres "
        "canónicos (Luis Rojas, Daniel Reyna, Manuel García, Paola Peña, Nicole Sereno). "
        "Usuarios de GitHub mapeados por tabla fija. Los PRs cerrados sin merge cuentan "
        "como trabajo completo del autor. El `mergedAt` se atribuye al autor del PR "
        "(simplificación: el merge lo ejecuta a veces el mantenedor). Los issues se "
        "atribuyen a sus asignados; si el issue no tiene label de sprint, se ubica por su "
        "fecha de creación.")
    add("")
    add("**Exclusiones:** `build*/`, `.agents_backup/`, binarios (no cuentan como archivos "
        "creados ni líneas), commits alcanzables solo vía `refs/stash`, y cualquier evento "
        "posterior al corte (%s)." % cutoff.isoformat())
    add("")
    add("**Proxy de creación de archivos:** primer commit (más antiguo, sobre `--all`) que "
        "añade el path.")
    add("")
    add("**Comandos clave:** `git log --all --use-mailmap --date=iso-strict`, "
        "`git log --all --diff-filter=A`, `git log --all --numstat`, `git shortlog -sne "
        "--all`, `gh pr list`, `gh issue list`, `gh pr view --json reviews,comments`.")
    add("")

    add("## Apéndice B — Horas por escenario (promedio de horas/día por sprint)")
    add("")
    add("| Persona | Sprint | 45 min | 60 min | 90 min | Días con fuente de horas |")
    add("|---|---|---|---|---|---|")
    for person in personas:
        for sprint_name, _, _ in sprints:
            h = hours.get(person, {}).get(sprint_name)
            if h is None:
                continue
            add("| %s | %s | %.1f | %.1f | %.1f | %d |" % (
                person, sprint_name, h["scen"][hours_scen[0]], h["scen"][hours_scen[1]],
                h["scen"][hours_scen[2]], h["n"]))
    add("")
    add("Horas totales estimadas por persona y sprint (rango entre escenarios):")
    add("")
    add("| Persona | Sprint | Horas totales (rango) |")
    add("|---|---|---|")
    for person in personas:
        for sprint_name, _, _ in sprints:
            h = hours.get(person, {}).get(sprint_name)
            if h is None:
                continue
            vals = [h["scen"][s] for s in hours_scen]
            add("| %s | %s | %s |" % (person, sprint_name,
                                      fmt_hours_range(min(vals) * h["n"],
                                                      max(vals) * h["n"])))
    add("")

    add("## Apéndice C — Volumen de líneas de código (texto, excluye build*/binarios)")
    add("")
    add("| Persona | Sprint | + líneas | − líneas |")
    add("|---|---|---|---|")
    for person in sorted(lines_vol):
        for sprint_name, _, _ in sprints:
            d = lines_vol[person].get(sprint_name)
            if d and (d["add"] or d["del"]):
                add("| %s | %s | %d | %d |" % (person, sprint_name, d["add"], d["del"]))
    add("")

    add("## Limitaciones (caveats)")
    add("")
    add("- **No son horas reales**: modelo estadístico de estimación con incertidumbre "
        "30–50%; sirve para comparar intensidad relativa, no para reportes oficiales.")
    add("- El mtime local es solo corroboración de la máquina de Luis Rojas; el trabajo "
        "sin eventos (reuniones, diseño en pizarra, lectura) es invisible para el modelo.")
    add("- La actividad de Luis puede estar sobre-representada (commits de CI/build/AI "
        "tooling, merges de mantenedor atribuidos a autores de PR).")
    add("- S1 (≤ 06-28) solo tiene actividad de bootstrap: Luis Rojas y Daniel Reyna; "
        "para el resto no hay datos (no se estima horas).")
    add("- Issues #45 y #55 quedaron fuera del rango #6–41 del diseño (no tienen labels de "
        "sprint); su actividad no se refleja en las métricas de issues.")
    add("- Projects v2: %s" % ("disponible" if projects_note[0] == "ok" else "excluido "
        "(token sin scope `read:project`)"))
    add("")

    add("## Verificación y reproducibilidad")
    add("")
    add("- Commits (`git rev-list --all --count`): %d" % total_commits)
    add("- Autores canónicos (`.mailmap`, `git shortlog -sne --all`): %d (%s)" % (
        len(authors), ", ".join(sorted(authors))))
    add("- PRs merged: %d | PRs cerrados sin merge: %d" % (merged_n, closed_n))
    add("- Reproducibilidad: `python3 scripts/traqueo-horas-por-sprint.py` dos veces "
        "consecutivas debe producir `diff` vacío (el informe no lleva marca de tiempo).")

    with open(out_path, "w", encoding="utf-8") as fh:
        fh.write("\n".join(L) + "\n")
    return L


def main(argv=None):
    ap = argparse.ArgumentParser(
        prog="traqueo-horas-por-sprint.py",
        description="Análisis read-only de actividad por sprint (git + GitHub + informes).")
    ap.add_argument("--cutoff", default="2026-08-01", metavar="YYYY-MM-DD",
                    help="Fecha de corte; eventos posteriores se excluyen (default 2026-08-01).")
    ap.add_argument("--gap-min", type=int, default=60, metavar="MIN",
                    help="Umbral de gap en minutos (default 60; escenarios = ±15).")
    ap.add_argument("--out", default=None, metavar="PATH",
                    help="Ruta del informe (default docs/analisis-actividad/traqueo-horas-por-sprint-<cutoff>.md).")
    args = ap.parse_args(argv)

    cutoff = dt.date.fromisoformat(args.cutoff)
    if cutoff < dt.date(2026, 7, 27):
        sys.exit("ERROR: el cutoff debe ser >= 2026-07-27 (inicio de S6).")
    if args.gap_min < 15:
        sys.exit("ERROR: --gap-min debe ser >= 15.")
    sprints = build_sprints(cutoff)
    out_path = args.out
    if out_path is None:
        out_path = os.path.join(ROOT, "docs", "analisis-actividad",
                                "traqueo-horas-por-sprint-%s.md" % cutoff.isoformat())
    elif not os.path.isabs(out_path):
        out_path = os.path.join(ROOT, out_path)

    notas = []
    tz = detect_repo_tz()

    # --- git ---
    total_commits = int(run(["git", "rev-list", "--all", "--count"]).strip())
    authors = shortlog_authors()
    stash_only = stash_only_commits()
    commits = git_commits(stash_only, tz)
    creations = git_creations(stash_only, tz)
    numstat = git_numstat(stash_only, tz)

    # --- github ---
    prs = gh_prs()
    issues = gh_issues()
    review_events = gh_pr_review_events(prs)
    projects_note = gh_projects_v2_note()

    merged = [p for p in prs if p.get("mergedAt")]
    closed_prs = [p for p in prs if not p.get("mergedAt")]

    # --- eventos y agregación ---
    global all_events
    all_events = build_events(commits, prs, issues, review_events, tz, sprints)
    days = aggregate(all_events, sprints)
    hours, hours_scen = compute_hours(all_events, sprints, args.gap_min)
    volume = compute_volume(commits, creations, prs, issues, sprints)
    lines_vol = lines_volume(numstat, sprints)

    review_tot = {}
    for e in all_events:
        if e["src"] != "review":
            continue
        sprint = sprint_of(e["day"], sprints)
        if sprint is None:
            continue
        review_tot.setdefault(e["persona"], {}).setdefault(sprint, 0)
        review_tot[e["persona"]][sprint] += 1

    # --- validación cruzada ---
    informes, notas_inf = parse_informes(sprints)
    notas.extend(notas_inf)
    cross_validate(informes, days, notas)

    # --- corroboración local ---
    local_days = local_mtime_days(out_path, tz)
    tl_info = timelapse_anchors()
    for d, sprint, exists in tl_info:
        if exists and not days.get("Luis Rojas", {}).get(sprint):
            notas.append("Timelapse %s (%s) existe pero no se evidencia actividad de "
                         "Luis Rojas en ese sprint." % (d.isoformat(), sprint))

    os.makedirs(os.path.dirname(out_path), exist_ok=True)
    render_report(out_path, args, tz, sprints, days, hours, hours_scen, volume,
                  lines_vol, informes, notas, review_tot, projects_note, local_days,
                  tl_info, len(merged), len(closed_prs), total_commits, authors,
                  closed_prs, sorted(stash_only), cutoff)

    # --- verificación (salida estándar) ---
    ok = lambda v, e: "OK " if v == e else "DIF"
    print("traqueo-horas-por-sprint: informe generado en %s" % out_path)
    print("VERIFICACIÓN:")
    print("  commits git --all ......... %s (%d, esperado 156)" % (ok(total_commits, 156), total_commits))
    print("  autores canónicos ......... %s (%d, esperado 5)" % (ok(len(authors), 5), len(authors)))
    print("  PRs merged ................ %s (%d, esperado 26)" % (ok(len(merged), 26), len(merged)))
    print("  PRs cerrados sin merge .... %d" % len(closed_prs))
    print("  issues en rango #6-41 ..... %d" % len(issues))
    print("  commits excluidos (stash) . %d" % len(stash_only))
    print("  proyectos v2 .............. %s" % projects_note[0])
    print("  discrepancias registradas . %d" % sum(1 for n in notas if n.startswith("Discrepancia")))
    if total_commits != 156 or len(authors) != 5 or len(merged) != 26:
        print("AVISO: algún total difiere del estado verificado — revisar data del repo/gh.")
    return 0


if __name__ == "__main__":
    sys.exit(main())


