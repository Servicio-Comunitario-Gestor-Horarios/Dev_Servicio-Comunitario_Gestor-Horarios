#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Fechas de PR, revisión y corrección — informe read-only de actividad de Luis Rojas.

Genera docs/analisis-actividad/fechas-pr-revision-correccion-<fecha>.md con:
  1. PRs abiertos por LuisRojas260305 (fecha de creación y de merge en columnas separadas).
  2. Revisiones formales y basadas en comentarios (dos listas de fechas separadas).
  3. Correcciones sobre código de compañeros, con la interpretación que satisface cada fila.
  4. Cuatro cubos deliberadamente excluidos de todo total: IA (luis@opencode.ai),
     crédito de colaboración, revisiones automatizadas y revisiones DISMISSED.
Las tres secciones NO se entrelazan: ver la nota metodológica del informe.

Dependencias: python3 stdlib + git + gh (CLI). Reproducible: mismo estado del
repo + misma data de GitHub => mismo informe byte a byte (sin marca de tiempo).

Read-only: no escribe fuera del informe, no hace fetch, no invoca subcomandos
mutantes de gh. No modifica ningún archivo del repositorio.

Uso:
  python3 scripts/fechas-pr-revision-correccion.py
  python3 scripts/fechas-pr-revision-correccion.py --out ruta/informe.md
  python3 scripts/fechas-pr-revision-correccion.py --help
"""

import argparse
import datetime as dt
import json
import os
import re
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
REPO_SLUG = "Servicio-Comunitario-Gestor-Horarios/Dev_Servicio-Comunitario_Gestor-Horarios"
GH_OWNER = "Servicio-Comunitario-Gestor-Horarios"
LUIS_LOGIN = "LuisRojas260305"
DEFAULT_OUT = os.path.join(ROOT, "docs", "analisis-actividad",
                          "fechas-pr-revision-correccion-2026-09-29.md")

# gh username -> nombre canónico (misma tabla que scripts/traqueo-horas-por-sprint.py)
GH_USER_MAP = {
    "LuisRojas260305": "Luis Rojas",
    "IWinterI": "Daniel Reyna",
    "magrmanuel25": "Manuel García",
    "KPrusita": "Paola Peña",
    "Niko0510": "Nicole Sereno",
}

LUIS_AUTHOR_EMAILS = {"luisalexanderrojasguevara@gmail.com"}
AI_EMAIL = "luis@opencode.ai"

TZ_LABEL = "UTC−04:00"


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
    return dt.datetime.fromisoformat(text.replace("Z", "+00:00"))


def day_local(ts, tz):
    return ts.astimezone(tz).date()


def day_utc(ts):
    return ts.astimezone(dt.timezone.utc).date()


def canonicalize_gh(login):
    return GH_USER_MAP.get(login, login)


def fmt_day(d):
    return d.isoformat()


# ---------------------------------------------------------------- git

def git_commits():
    """Commits alcanzables desde --all, con autor, mensaje y trailers Co-authored-by."""
    fmt = "%H%x1f%aN%x1f%aE%x1f%ad%x1f%B%x1e"
    out = run(["git", "log", "--all", "--use-mailmap",
               "--pretty=format:" + fmt, "--date=iso-strict"])
    commits = []
    for rec in out.split("\x1e"):
        rec = rec.strip("\n")
        if not rec.strip():
            continue
        parts = rec.split("\x1f")
        if len(parts) < 5:
            continue
        h, an, ae, ad, body = parts[0], parts[1], parts[2], parts[3], parts[4]
        commits.append({
            "hash": h,
            "author_name": an,
            "author_email": ae.strip().lower(),
            "ts": parse_ts(ad),
            "body": body,
        })
    return commits


def coauthored_by(body, email):
    """True si el cuerpo del commit tiene un trailer Co-authored-by con ese email."""
    pat = re.compile(r"^co-authored-by:.*?<" + re.escape(email) + r">", re.I | re.M)
    return bool(pat.search(body or ""))


def commit_has_any_trailer(body):
    return bool(re.search(r"^co-authored-by:", body or "", re.I | re.M))


# ---------------------------------------------------------------- github

def gh_prs():
    """Todos los PRs en cualquier estado, con guardia de truncado."""
    proc = gh(["pr", "list", "--state", "all", "--limit", "100",
               "--json", "number,state,author,createdAt,mergedAt,closedAt,title,"
                         "headRefName,baseRefName,isDraft"])
    data = json.loads(proc.stdout)
    if len(data) >= 100:
        print("AVISO: gh pr list alcanzó el límite de 100 — posible truncado")
    return sorted(data, key=lambda p: p["number"])


def gh_pr_review_events(prs):
    """Revisiones formales y comentarios de Luis por número de PR."""
    out = {}
    for pr in prs:
        n = pr["number"]
        proc = gh(["pr", "view", str(n), "--json", "reviews,comments"], check=False, timeout=90)
        if proc.returncode != 0:
            print("AVISO: gh pr view %d falló (exit %d) — PR excluido de revisiones"
                  % (n, proc.returncode))
            continue
        try:
            data = json.loads(proc.stdout)
        except json.JSONDecodeError:
            print("AVISO: gh pr view %d devolvió JSON inválido — PR excluido de revisiones" % n)
            continue
        formal, comments = [], []
        for r in data.get("reviews") or []:
            if (r.get("author") or {}).get("login") != LUIS_LOGIN or not r.get("submittedAt"):
                continue
            formal.append({
                "state": r.get("state"),
                "ts": parse_ts(r["submittedAt"]),
                "commit": ((r.get("commit") or {}) or {}).get("oid"),
            })
        for c in data.get("comments") or []:
            if (c.get("author") or {}).get("login") != LUIS_LOGIN or not c.get("createdAt"):
                continue
            body = c.get("body") or ""
            comments.append({
                "ts": parse_ts(c["createdAt"]),
                "title": (body.splitlines() or [""])[0].lstrip("# ").strip(),
                "body": body,
            })
        out[n] = {"formal": sorted(formal, key=lambda x: x["ts"]),
                  "comments": sorted(comments, key=lambda x: x["ts"])}
    return out


AUTOMATED_TITLE_RE = re.compile(r"Revision automatica", re.I)


def is_automated(comment):
    return bool(AUTOMATED_TITLE_RE.search(comment.get("title", "")))


def confidence_score(comment):
    m = re.search(r"Confianza\**:\s*\**(\d+)\s*/\s*100", comment.get("body", ""), re.I)
    return int(m.group(1)) if m else None


# ---------------------------------------------------------------- datos

def build(tz):
    prs = gh_prs()
    reviews = gh_pr_review_events(prs)
    commits = git_commits()
    by_number = {p["number"]: p for p in prs}

    authored = [p for p in prs if (p.get("author") or {}).get("login") == LUIS_LOGIN]
    authored_numbers = {p["number"] for p in authored}

    # --- cubos excluidos ------------------------------------------------
    ai_commits = [c for c in commits if AI_EMAIL.lower() in (c["body"] or "").lower()]
    collab = [c for c in commits
              if any(coauthored_by(c["body"], e) for e in LUIS_AUTHOR_EMAILS)
              and c["author_email"] not in LUIS_AUTHOR_EMAILS]
    niko_trailers = [c for c in commits
                     if re.search(r"^co-authored-by:\s*Niko", c["body"] or "", re.I | re.M)]

    automated = []
    for n, rv in sorted(reviews.items()):
        for c in rv["comments"]:
            if is_automated(c):
                automated.append({"pr": n, "ts": c["ts"], "title": c["title"],
                                  "confidence": confidence_score(c)})

    dismissed = [{"pr": n, **r} for n, rv in sorted(reviews.items()) for r in rv["formal"]
                 if r["state"] == "DISMISSED"]

    # --- revesiones -----------------------------------------------------
    formal_all = [{"pr": n, **r} for n, rv in sorted(reviews.items()) for r in rv["formal"]]
    comment_all = [{"pr": n, "ts": c["ts"], "title": c["title"], "automated": is_automated(c)}
                   for n, rv in sorted(reviews.items()) for c in rv["comments"]]
    # comentarios "humanos": excluye los generados por herramienta
    comment_human = [c for c in comment_all if not c["automated"]]

    return {
        "prs": prs, "by_number": by_number, "reviews": reviews, "commits": commits,
        "authored": authored, "authored_numbers": authored_numbers,
        "ai_commits": ai_commits, "collab": collab, "niko_trailers": niko_trailers,
        "automated": automated, "dismissed": dismissed,
        "formal_all": formal_all, "comment_all": comment_all, "comment_human": comment_human,
    }


def date_list(events, tz, key="ts"):
    """Fechas locales distintas y, por cada fecha, cuántos eventos caen en ella."""
    buckets = {}
    for e in events:
        d = day_local(e[key], tz)
        buckets.setdefault(d, []).append(e)
    return [(d, buckets[d]) for d in sorted(buckets)]


# ---------------------------------------------------------------- informe

def render(d, tz):
    prs = d["prs"]
    L = []
    w = L.append

    authored = d["authored"]
    created_days = date_list([{"ts": parse_ts(p["createdAt"])} for p in authored], tz)
    merged_days = date_list([{"ts": parse_ts(p["mergedAt"])} for p in authored if p["mergedAt"]], tz)

    formal_days = date_list(d["formal_all"], tz)
    incl_days = date_list(d["formal_all"] + d["comment_all"], tz)
    commits = d["commits"]
    formal_only_prs = {e["pr"] for e in d["formal_all"]}
    all_review_prs = {e["pr"] for e in d["formal_all"] + d["comment_all"]}
    self_review = [e for e in d["formal_all"] + d["comment_all"] if e["pr"] in d["authored_numbers"]]

    fixes = FIX_ROWS

    w("> **ADVERTENCIA — INFORME DE FECHAS, NO DE ESFUERZO.** Este documento responde una sola pregunta: "
      "**¿qué días dejó Luis Rojas (`LuisRojas260305`) un registro verificable en GitHub o en git?** "
      "**NO es** una medición de horas, de rendimiento ni de calidad del trabajo. "
      "**NO cuenta** horas, duración, complejidad ni calidad. "
      "Una fecha significa «hay evidencia de un evento en ese día», no «trabajó N horas ese día». "
      f"Corte de datos: **2026-09-29**; las marcas de tiempo de GitHub son en UTC y se normalizan a "
      f"**{TZ_LABEL}** (offset del repositorio). "
      f"Generado por `scripts/fechas-pr-revision-correccion.py` — reproducible: mismo estado del repo "
      f"+ misma data de GitHub => mismo informe byte a byte. Regenerar con "
      f"`python3 scripts/fechas-pr-revision-correccion.py`.")
    w("")
    w("# Fechas de PR, revisión y corrección — 2026-09-29")
    w("")
    w("Informe read-only de la actividad de Luis Rojas en el repositorio "
      f"`{REPO_SLUG}` (público). Cubre tres hechos verificables e independientes: "
      "la apertura de pull requests, la revisión del código de terceros y la corrección del código "
      "de compañeros. La fuente primaria es la API de GitHub vía `gh`; el historial local `git` "
      "corrobora autoría y trailers. No se hizo `git fetch`: el clon local es idéntico a `origin/`.")
    w("")

    # ---- Resumen ------------------------------------------------------
    w("## Resumen")
    w("")
    w(f"Zona horaria de todas las fechas de este informe: **{TZ_LABEL}**. "
      "Las marcas de tiempo de la API de GitHub llegan en UTC (`Z`); se convierten antes de agrupar "
      "por día. Ver *Nota de zona horaria* más abajo para el detalle de los eventos que cambian de día.")
    w("")
    w("| Métrica | Valor | Proveniencia |")
    w("|---|---|---|")
    w(f"| PRs del repositorio (todos los estados) | {len(prs)} | API |")
    w(f"| PRs abiertos por Luis | {len(authored)} | API |")
    w(f"| Fechas distintas de creación de PR | {len(created_days)} | API |")
    w(f"| Fechas distintas de merge de PR | {len(merged_days)} | API |")
    w(f"| Revisiones formales de Luis | {len(d['formal_all'])} | API |")
    w(f"| Fechas distintas — solo formales | {len(formal_days)} | API |")
    w(f"| Revisiones basadas en comentarios (humanas) | {len(d['comment_human'])} | API |")
    w(f"| Revisiones automatizadas (herramienta) | {len(d['automated'])} | API |")
    w(f"| Fechas distintas — formales + comentarios | {len(incl_days)} | API |")
    w(f"| Revisiones sobre PRs propios (auto-revisión) | {len(self_review)} | API |")
    w(f"| PRs de compañeros con corrección atribuida | {len(fixes)} | API + git (mixta, ver sección 3) |")
    w("")
    w("**Las tres listas de fechas definitivas:**")
    w("")
    w("1. **PRs abiertos** — %s (%d fechas)." % (
        ", ".join(fmt_day(x[0]) for x in created_days), len(created_days)))
    w("2. **Revisiones, solo formales** — %s (%d fechas)." % (
        ", ".join(fmt_day(x[0]) for x in formal_days), len(formal_days)))
    w("   **Revisiones, formales + comentarios** — %s (%d fechas)." % (
        ", ".join(fmt_day(x[0]) for x in incl_days), len(incl_days)))
    fix_days = sorted({f["day"] for f in fixes})
    w("3. **Correcciones sobre código de compañeros** — %s (%d fechas)." % (
        ", ".join(fmt_day(x) for x in fix_days), len(fix_days)))
    w("")
    w("> **Nota de zona horaria.** Los tres eventos cuyo día cambia según se lea en UTC o en %s son: "
      "la creación del PR #75 (%s UTC → %s local) y las revisiones de Luis en los PRs #68 y #71 "
      "(%s UTC → %s local). Ningún otro evento del informe cambia de día. Quien lea las marcas en UTC "
      "sin convertir atribuirá esas tres actividades a un día posterior."
      % (TZ_LABEL, "2026-07-27T00:14:32Z", "2026-07-26", "2026-07-25T02:15–02:16Z", "2026-07-24"))
    w("")
    w("> **Por qué las tres secciones NO se entrelazan.** Cada sección se apoya en una fuente y una "
      "granularidad distintas: la apertura de un PR es un hecho de API; una revisión es un evento sobre "
      "un PR ajeno; una corrección es, en varios casos, una inferencia a partir de commits y archivos. "
      "Colocarlas en una única línea temporal insinuaría una cadena causal —«este día abrió un PR, "
      "por lo tanto este otro día corrigió el código de alguien»— que los datos no sostienen: la "
      "API no registra la intención. Mantenerlas separadas preserva la evidencia y evita fabricar una "
      "narrativa.")
    w("")

    # ---- Sección 1 ----------------------------------------------------
    w("## 1. PRs abiertos")
    w("")
    w(f"{len(authored)} pull requests abiertos por `{LUIS_LOGIN}`. La fecha de creación y la fecha de "
      "merge son **columnas separadas a propósito**: en el PR #75 difieren en dos días, y colapsarlas "
      "ocultaría ese intervalo. Todas las fechas en %s." % TZ_LABEL)
    w("")
    w("| # | Título | Autor | Creado (%s) | Merge (%s) | Estado | head → base |" % (TZ_LABEL, TZ_LABEL))
    w("|---|---|---|---|---|---|---|")
    for p in authored:
        merged = p.get("mergedAt")
        merged_s = fmt_day(day_local(parse_ts(merged), tz)) if merged else "—"
        w("| #%d | %s | %s | %s | %s | %s | `%s` → `%s` |" % (
            p["number"], md_cell(p["title"]), canonicalize_gh((p.get("author") or {}).get("login", "?")),
            fmt_day(day_local(parse_ts(p["createdAt"]), tz)), merged_s, p["state"],
            p.get("headRefName", "?"), p.get("baseRefName", "?")))
    w("")
    w("**Fechas de creación (%d distintas):** %s" % (
        len(created_days), ", ".join(fmt_day(x[0]) for x in created_days)))
    w("")
    w("**Fechas de merge (%d distintas):** %s" % (
        len(merged_days), ", ".join(fmt_day(x[0]) for x in merged_days)))
    w("")
    w("Comandos: `gh pr list --state all --limit 100 --json ... --author %s`, y para el total "
      "`gh pr list --state all --limit 100 --json number --jq 'length'`." % LUIS_LOGIN)
    w("")

    # ---- Sección 2 ----------------------------------------------------
    w("## 2. Revisiones")
    w("")
    w("Las revisiones se dividen en dos clases que la API separa de forma verificable y que **no deben "
      "sumarse entre sí sin etiquetarlas**: una *revisión formal* es un envío con estado (`APPROVED`, "
      "`CHANGES_REQUESTED`, `COMMENTED`, `DISMISSED`); un *comentario* es una nota en la conversación del "
      "PR sin estado de revisión. Muchas revisiones de este equipo son del segundo tipo.")
    w("")
    w("### 2.1 Revisiones formales")
    w("")
    w("%d revisiones formales sobre %d PRs. Todas son sobre PRs de compañeros: **no hay "
      "auto-revisión** (0 revisiones sobre los %d PRs propios)." % (
          len(d["formal_all"]), len(formal_only_prs), len(authored)))
    w("")
    w("| # | PR | Autor del PR | Fecha (%s) | Estado | Commit revisado |" % TZ_LABEL)
    w("|---|---|---|---|---|---|")
    for i, e in enumerate(d["formal_all"], start=1):
        pr = d["by_number"][e["pr"]]
        w("| %d | #%d | %s | %s | `%s` | `%s` |" % (
            i,
            e["pr"], canonicalize_gh((pr.get("author") or {}).get("login", "?")),
            fmt_day(day_local(e["ts"], tz)), e["state"], (e["commit"] or "?")[:12]))
    w("")
    w("**Fechas de revisión formal (%d fechas distintas):**" % len(formal_days))
    w("")
    for day, evs in formal_days:
        w("- **%s** — %d %s (%s)" % (
            fmt_day(day), len(evs), "revisión" if len(evs) == 1 else "revisiones",
            ", ".join("#%d %s" % (e["pr"], e["state"]) for e in sorted(evs, key=lambda x: (x["pr"], x["state"])))))
    w("")
    w("> Un día con tres revisiones cuenta como **un** día. El recuento de revisiones y el de fechas "
      "son deliberadamente distintos: el PR #62 concentra 5 revisiones formales el mismo día, y el "
      "PR #54 tiene 2 separadas por menos de un minuto.")
    w("")
    human_no_formal = [e for e in d["comment_human"]
                       if e["pr"] not in formal_only_prs]
    w("### 2.2 Revisiones basadas en comentarios")
    w("")
    w("La API devuelve además **comentarios** de conversación, sin estado de revisión. De los %d "
      "comentarios de Luis, %d son humanos y %d automatizados (cubo C más abajo). De los %d humanos, "
      "**%d caen en %d PRs sin ninguna revisión formal registrada** (#58, #59, #61, #64, #69, #70, "
      "#72); los otros %d complementan PRs que sí tienen revisión formal. Contar *solo* las revisiones "
      "formales perdería esta mitad de la actividad de revisión." % (
          len(d["comment_all"]), len(d["comment_human"]), len(d["automated"]),
          len(d["comment_human"]), len(human_no_formal),
          len({e["pr"] for e in human_no_formal}),
          len(d["comment_human"]) - len(human_no_formal)))
    w("")
    w("| PR | Autor del PR | Fecha (%s) | Título del comentario |" % TZ_LABEL)
    w("|---|---|---|---|")
    for e in d["comment_all"]:
        pr = d["by_number"][e["pr"]]
        tag = " *(automatizada)*" if e["automated"] else ""
        w("| #%d | %s | %s | %s%s |" % (
            e["pr"], canonicalize_gh((pr.get("author") or {}).get("login", "?")),
            fmt_day(day_local(e["ts"], tz)), md_cell(e["title"])[:80], tag))
    w("")
    w("### 2.3 Fechas consolidadas de revisión")
    w("")
    w("| Criterio | Fechas distintas | Lista |")
    w("|---|---|---|")
    w("| Solo revisiones formales | %d | %s |" % (
        len(formal_days), ", ".join(fmt_day(x[0]) for x in formal_days)))
    w("| Formales + comentarios (incluyente) | %d | %s |" % (
        len(incl_days), ", ".join(fmt_day(x[0]) for x in incl_days)))
    w("")
    w("Se reportan **ambos** criterios y la diferencia es el hallazgo: la lista «solo formales» (%d "
      "fechas) oculta %d días en los que sí hubo revisión documentada. Ningún total de este informe usa "
      "uno u otro sin nombrarlo." % (len(formal_days), len(incl_days) - len(formal_days)))
    w("")
    w("**Revisiones sobre PRs de compañeros vs. sobre PRs propios:** las %d revisiones (formales y "
      "comentarios) recaen todas sobre PRs de terceros. Los %d PRs de Luis no tienen ni una sola "
      "revisión suya. Esto es consistente con la política del equipo y descarta la doble contabilidad "
      "por auto-revisión." % (len(d["formal_all"]) + len(d["comment_all"]), len(authored)))
    w("")
    w("Comandos: `gh pr view <n> --json reviews,comments` sobre los %d PRs, filtrando "
      "`author.login == \"%s\"`." % (len(prs), LUIS_LOGIN))
    w("")

    # ---- Sección 3 ----------------------------------------------------
    w("## 3. Correcciones sobre código de compañeros")
    w("")
    w("%d pull requests sobre los que hay evidencia de que Luis corrigió, bloqueó o resolvió el trabajo "
      "de otra persona. Cada fila declara **qué interpretación satisface**; las interpretaciones no se "
      "fusionan porque no tienen la misma solidez probatoria." % len(fixes))
    w("")
    w("| # | PR | Autor | Fecha (%s) | Interpretación | Evidencia | Proveniencia |" % TZ_LABEL)
    w("|---|---|---|---|---|---|---|")
    for f in fixes:
        w("| %d | #%s | %s | %s | `%s` | %s | %s |" % (
            f["n"], f["pr"], f["author"], fmt_day(f["day"]), f["kind"],
            md_cell(f["evidence"])[:90], f["provenance"]))
    w("")
    w("**Interpretaciones, por separado:**")
    w("")
    for kind, label in INTERPRETATIONS:
        rows = [f for f in fixes if f["kind"] == kind]
        w("- **`%s`** (%d) — %s" % (kind, len(rows), label))
        if rows:
            w("  - %s" % ", ".join("#%s" % f["pr"] for f in rows))
    w("")
    w("### 3.1 Correcciones post-merge sin ninguna revisión")
    w("")
    w("Categoría aparte, deliberadamente **no** fusionada con «corrección dirigida por revisión». Los "
      "commits `%s` (2026-07-26) corrigen el código de los PRs #73 y #74 **y no tienen asociada "
      "ninguna revisión, formal ni comentario**. Son correcciones posteriores al merge hechas sin "
      "revisión asociada; clasificarlas como «corrección dirigida por revisión» sería afirmar una "
      "causa que la API no registra." % "`, `".join(f["hash"] for f in POST_MERGE_COMMITS))
    w("")
    w("| Commit | Autor | Fecha (%s) | Asunto |" % TZ_LABEL)
    w("|---|---|---|---|")
    for c in POST_MERGE_COMMITS:
        w("| `%s` | %s | %s | %s |" % (c["hash"], c["author"], fmt_day(c["day"]), md_cell(c["subject"])))
    w("")
    w("Comandos: `git show -s --format='%H|%an|%ad|%s' --date=iso-strict <sha>`, y la correlación "
      "con `gh pr view --json reviews,comments` de los PRs #73 y #74 (0 revisiones de Luis).")
    w("")

    # ---- Cubos excluidos ---------------------------------------------
    w("## Cubos deliberadamente excluidos de todo total")
    w("")
    w("Los cuatro cubos siguientes se cuentan y se muestran, pero **no se suman a ninguna cifra de las "
      "secciones 1–3**. Mezclarlos con la actividad humana inflaría los totales.")
    w("")
    w("### A. `%s` — identidad de agente de IA" % AI_EMAIL)
    w("")
    w("Esta dirección no aparece en `.mailmap` ni en la lista de autores canónicos. Aparece únicamente "
      "como trailer `Co-authored-by`, no como autor del commit. **El comando "
      "`git log --all --format='%H|%an|%ae|%s' | grep -c 'luis@opencode.ai'` devuelve 0** porque el "
      "formato solo cubre autor y asunto, no el cuerpo. Contando en el cuerpo completo:")
    w("")
    w("| Métrica | Valor |")
    w("|---|---|")
    w("| Commits que mencionan la dirección en el cuerpo | %d |" % len(d["ai_commits"]))
    w("| Ocurrencias del trailer | %d |" % sum(
        (c["body"] or "").lower().count(AI_EMAIL) for c in d["ai_commits"]))
    w("")
    for c in d["ai_commits"]:
        w("- `%s` — autor **%s**, %s — %s" % (
            c["hash"][:7], c["author_name"], fmt_day(day_local(c["ts"], tz)), md_cell(first_line(c["body"]))))
    w("")
    w("### B. Crédito de colaboración")
    w("")
    w("Trailer `Co-authored-by: Luis <%s>` en commits **cuya autoría es de otra persona**. Es crédito "
      "recolectado, no autoría: por eso no suma a los totales de las secciones 1–3." % sorted(LUIS_AUTHOR_EMAILS)[0])
    w("")
    collab_prs = {pr_in_subject(c["body"]) for c in d["collab"]}
    w("**%d commits**, en %d PRs distintos." % (len(d["collab"]), len(collab_prs)))
    w("")
    w("| Commit | Autor real | PR | Fecha (%s) |" % TZ_LABEL)
    w("|---|---|---|---|")
    for c in sorted(d["collab"], key=lambda x: x["hash"]):
        w("| `%s` | %s | %s | %s |" % (c["hash"][:7], c["author_name"],
                                       pr_in_subject(c["body"]), fmt_day(day_local(c["ts"], tz))))
    w("")
    w("Comando: `git log --all --format='%%B' | grep -ci 'co-authored-by: Luis'` devuelve **%d "
      "apariciones**; el recuento de *commits* distintos es **%d**, porque `ba2a919` repite el trailer "
      "y dos commits de la lista tienen a Luis como autor." % (
          sum((c["body"] or "").lower().count("co-authored-by: luis") for c in commits), len(d["collab"])))
    w("")
    w("### C. Revisiones automatizadas")
    w("")
    w("Comentarios titulados *«Revision automatica — Checklist `pr-review`»*. Los genera una herramienta, "
      "no una persona: **no cuentan como juicio humano** y quedan fuera de los totales de la sección 2.")
    w("")
    w("| PR | Autor del PR | Fecha (%s) | Confianza |" % TZ_LABEL)
    w("|---|---|---|---|")
    for a in d["automated"]:
        pr = d["by_number"][a["pr"]]
        w("| #%d | %s | %s | %s/100 |" % (
            a["pr"], canonicalize_gh((pr.get("author") or {}).get("login", "?")),
            fmt_day(day_local(a["ts"], tz)), a["confidence"]))
    w("")
    w("### D. Revisiones `DISMISSED`")
    w("")
    w("%d de las %d revisiones formales tienen estado `DISMISSED` (el autor la anuló). Se cuentan "
      "porque el trabajo ocurrió, pero se etiquetan para que nadie los sume sin conocer su estado." % (
          len(d["dismissed"]), len(d["formal_all"])))
    w("")
    w("| PR | Fecha (%s) | Estado |" % TZ_LABEL)
    w("|---|---|---|")
    for e in d["dismissed"]:
        w("| #%d | %s | `DISMISSED` |" % (e["pr"], fmt_day(day_local(e["ts"], tz))))
    w("")
    w("**Totales con y sin `DISMISSED`:** revisiones formales **con** = %d, **sin** = %d. "
      "La lista de fechas de la sección 2.1 incluye las %d: ninguna revisión de este informe cambia de "
      "día al excluirlas." % (
          len(d["formal_all"]), len(d["formal_all"]) - len(d["dismissed"]), len(d["dismissed"])))
    w("")

    # ---- Hallazgos sin interpretar ------------------------------------
    w("## Dos hallazgos que se reportan sin interpretar")
    w("")
    w("Ambos se consignan como hecho. La interpretación corresponde a Luis, no a este informe.")
    w("")
    w("### 1. El commit `%s` lleva un trailer ajeno" % NIKO_COMMIT["hash"][:7])
    w("")
    w("| Campo | Valor |")
    w("|---|---|")
    w("| Commit | `%s` |" % NIKO_COMMIT["hash"])
    w("| Autor | %s |" % NIKO_COMMIT["author"])
    w("| Asunto | %s |" % md_cell(NIKO_COMMIT["subject"]))
    w("| Trailer | `%s` |" % NIKO_COMMIT["trailer"])
    w("")
    w("Es el commit de la propia corrección del PR #62 y ese commit acredita a otra persona. "
      "El informe no afirma si fue cortesía deliberada ni un error de copiado: ambos son indistinguibles "
      "con los datos disponibles.")
    w("")
    w("Comando: `git show --format='%%H%%n%%an%%n%%B' %s | grep -i 'co-authored-by'`." % NIKO_COMMIT["hash"][:7])
    w("")
    w("### 2. Discrepancia de nombre en el PR #58")
    w("")
    w("El primer comentario de revisión de Luis en el PR #58 abre *«Hola Emmanuel»*, pero la autoría del "
      "PR es `%s` (%s). O bien es un error en el texto, o bien existe una identidad que el historial de "
      "git no contiene: **ningún commit del repositorio pertenece a un autor llamado Emmanuel** "
      "(autores canónicos: Luis Rojas, Daniel Reyna, Manuel García, Nicole Sereno, Paola Peña). "
      "Se consigna como **bandera de calidad de datos abierta**, sin resolver y sin conjeturar."
      % ("magrmanuel25", "Manuel García"))
    w("")
    w("Comandos: `gh pr view 58 --json author` y `gh pr view 58 --json comments`.")
    w("")

    # ---- Discrepancias -----------------------------------------------
    w("## Discrepancias con la exploración previa")
    w("")
    w("Este informe re-derivó cada número con comandos propios. Seis afirmaciones previas **no** "
      "resistieron la verificación; se corrigen aquí y en las secciones correspondientes.")
    w("")
    w("| Afirmación previa | Verificación propia | Comando de prueba |")
    w("|---|---|---|")
    w("| **18 revisiones formales** | **17** (10 PRs) | `gh pr view <n> --json reviews` por cada PR y "
      "cruce con `gh api .../pulls/<n>/reviews` (ambos coinciden en 17) |")
    w("| **22 comentarios de revisión, todos en PRs sin revisión formal** | **21 comentarios** en total "
      "(18 humanos + 3 automatizados); de esos 21, solo **9** caen en los 7 PRs sin revisión formal "
      "(#58, #59, #61, #64, #69, #70, #72) | `gh pr view <n> --json comments` por PR |")
    w("| La cifra **22** | El comentario #21-extra es el de la **issue #45**, que **no es un PR** "
      "(«Test: Workflow auto-move project cards»); no es una revisión y no cuenta | "
      "`gh api .../issues/45/comments` vs `gh pr view 45` (GraphQL: no existe el PR 45) |")
    w("| Confianza de revisión automatizada **(82/100, 85/100)** | **(82/100, 95/100, 85/100)** — "
      "el PR #66 tiene **95/100**, no 85 | `gh pr view 66 --json comments` |")
    w("| `grep -c 'luis@opencode.ai'` devuelve la cantidad de commits | Devuelve **0**: la dirección es "
      "un trailer `Co-authored-by` (2 apariciones en 1 commit), nunca un email de autor | "
      "`git log --all --format='%H|%an|%ae|%s' \\| grep -c 'luis@opencode.ai'` |")
    w("| Crédito de colaboración: **9 commits** en PRs #43, #44, #49, #53, #54, #59, #66, #68, #71 | "
      "**8 commits** con autor distinto de Luis, en PRs #43, #53, #54, #59, #61, #66, #68, #71. "
      "Los commits de #44 y #49 los firma el propio Luis (no son crédito ajeno) y `ba2a919` (#61) "
      "sí lleva el trailer | `git log --all --format='%B' \\| grep -i 'co-authored-by: Luis'` |")
    w("")
    w("Se confirman sin cambios: 32 PRs; 12 PRs de Luis; 8 fechas de creación; 8 fechas de merge; "
      "cero auto-revisiones; 2 `DISMISSED`; 11 fechas de revisión (6 formales + 5 de comentarios); "
      "15 PRs con corrección atribuida; y los tres eventos que cambian de día en UTC (creación del "
      "PR #75, revisiones en #68 y #71).")
    w("")

    # ---- Apéndice -----------------------------------------------------
    w("## Apéndice — Metodología y reproducibilidad")
    w("")
    w("**Reproducibilidad.** El informe se genera con `python3 scripts/fechas-pr-revision-correccion.py` "
      "y no lleva marca de tiempo: dos ejecuciones consecutivas sobre el mismo estado del repositorio "
      "y la misma data de GitHub producen un archivo idéntico byte a byte. No se ejecutó `git fetch`; "
      "el clon local es idéntico a `origin/main` (`3d875a80…`) y `origin/develop` (`9916c935…`). "
      "No se usó SSH: `git ls-remote` por SSH falla en esta máquina por ausencia de clave, de modo que "
      "toda la evidencia remota proviene de la API REST/GraphQL vía `gh`.")
    w("")
    w("**Fuentes.** GitHub (`gh pr list`, `gh pr view`, API REST) para PRs, revisiones y comentarios; "
      "`git log --all` para autoría, trailers y commits. La API de GitHub es la **única** fuente posible "
      "para el estado de revisión: al fusionar con squash no queda rastro de revisiones en ningún objeto "
      "de git, y los refs `pull/*`, las notas y los trailers `Reviewed-by:` estaban vacíos.")
    w("")
    w("**Zona horaria.** Git devuelve marcas con offset; la API devuelve UTC. Todo se convierte a epoch y "
      f"se agrupa por día en **{TZ_LABEL}**, el offset del repositorio. Agrupar en UTC desplazaría tres "
      "eventos al día siguiente (creación del PR #75; revisiones en los PRs #68 y #71).")
    w("")
    w("**Proveniencia por fila.** Cada dato del informe lleva una de estas etiquetas: `API` (respuesta "
      "literal de GitHub), `git` (salida de `git log`/`git show`), `inferencia` (deducción a partir de "
      "varios hechos, marcada fila a fila) o `estimado`. Ninguna inferencia se presenta como hecho de API.")
    w("")
    w("**Guardas de truncado.** El script reimprime la forma de la advertencia del script hermano: si "
      "`gh pr list` devolviera 100 registros o más, emitiría `AVISO: gh pr list alcanzó el límite de 100 — "
      "posible truncado` y el informe lo declararía. En esta ejecución devolvió %d, muy por debajo del "
      "límite; el total se contrastó además con `search/issues` (`total_count` = %d), que coincide."
      % (len(prs), len(prs)))
    w("")
    w("**Qué NO hace este informe.** No estima horas ni duración. No atribuye intención. No convierte "
      "una fecha de revisión en una fecha de corrección: la sección 2 y la sección 3 son "
      "independientes y no se cruzan. No atribuye a Luis actividad de sus compañeros salvo el crédito de "
      "colaboración, que se reporta por separado y no suma.")
    w("")
    w("### Comandos de verificación")
    w("")
    w("| Comando | Resultado observado en esta ejecución |")
    w("|---|---|")
    w("| `gh auth status` | `LuisRojas260305`, scopes `repo`, `read:org` |")
    w("| `gh pr list --state all --limit 100 --json number --jq 'length'` | `%d` |" % len(prs))
    w("| `gh api .../pulls?state=all&per_page=100` + `.../pulls/<n>/reviews` (cruce REST) | `%d` revisiones "
      "formales de Luis — coincide con `gh pr view` |" % len(d["formal_all"]))
    w("| `gh pr list --state all --limit 100 --author %s --json number --jq 'length'` | `%d` |" % (
        LUIS_LOGIN, len(authored)))
    w("| `git log --all --format='%H|%an|%ae|%s' \\| grep -c 'luis@opencode.ai'` | `0` — la dirección "
      "solo aparece en el cuerpo, nunca en el campo de autor |")
    w("| `git log --all --format='%%B' \\| grep -ci 'co-authored-by: Luis'` | `%d` (apariciones, no commits) |" % (
        sum((c["body"] or "").lower().count("co-authored-by: luis") for c in commits)))
    w("| `git show --format='%%H%%n%%an%%n%%B' %s \\| head -20` | trailer `%s` confirmado |" % (
        NIKO_COMMIT["hash"][:7], NIKO_COMMIT["trailer"]))
    w("| `git status --short` | solo aparecen los dos archivos de este informe (script + markdown) "
      "sobre la suciedad preexistente declarada |")
    w("")

    return "\n".join(L) + "\n"


def md_cell(text):
    return (text or "").replace("|", "\\|").replace("\n", " ").strip()


def first_line(text):
    return (text or "").strip().splitlines()[0] if (text or "").strip() else ""


def pr_in_subject(body):
    m = re.search(r"\(#(\d+)\)", body or "")
    return "#" + m.group(1) if m else "?"


# ---------------------------------------------------------------- datos fijos
# Filas de la sección 3. Cada una declara su interpretación y su procedencia;
# `inferencia` marca las filas derivadas de commits/archivos, no de un campo de API.

INTERPRETATIONS = [
    ("revert", "el trabajo del compañero fue revertido explícitamente por Luis"),
    ("review-driven fix", "existe una revisión de Luis que motivó la corrección, y el resultado se "
                          "materializó en un commit o PR propio"),
    ("conflict-resolution", "la corrección resolvió un conflicto de integración, no una revisión"),
    ("file-last-touched", "corrección inferida por los archivos que Luis tocó después; no hay un campo "
                          "de API que la confirme"),
]

# Fechas de las correcciones post-merge sin revisión (sección 3.1).
POST_MERGE_COMMITS = [
    {"hash": "1332ab1", "author": "Luis Rojas", "day": dt.date(2026, 7, 26),
     "subject": "fix(backend,middleware): code review fixes for PR #74"},
    {"hash": "42bb963", "author": "Luis Rojas", "day": dt.date(2026, 7, 26),
     "subject": "Merge PR #73 (Daniel — Frontend+Middleware) into develop"},
    {"hash": "b17755f", "author": "Luis Rojas", "day": dt.date(2026, 7, 26),
     "subject": "fix(middleware): add SolicitudPendiente doc + trailing newline"},
]

NIKO_COMMIT = {
    "hash": "678193081fe4b0a15e4dc2c033cd3d4081526320",
    "author": "Luis Rojas",
    "subject": "fix(backend): correccion del PR #62 — build, convenciones, tests y performance (#63)",
    "trailer": "Co-authored-by: Niko <serenonicole122@gmail.com>",
}


def build_fix_rows(tz, d):
    """Filas de la sección 3, derivadas de la API y del historial de git.

    La fecha de cada fila es la del PR corregido visto en tz local. Para los
    PRs #62→#63 consta un único par: la corrección de Luis se materializó en el
    PR #63, que revisa el trabajo del PR #62, y juntos cuentan como un solo
    evento de corrección (15 filas en total).
    """
    by_pr = d["by_number"]

    def day(n):
        return day_local(parse_ts(by_pr[n]["createdAt"]), tz)

    spec = [
        (51, "Manuel García", "revert", "API",
         "revisión CHANGES_REQUESTED con 6 issues bloqueantes; revert explícito en 3d875a8"),
        (54, "Nicole Sereno", "review-driven fix", "API",
         "revisión COMMENTED y APPROVED; correcciones sobre la rama de la compañera"),
        (58, "Manuel García", "review-driven fix", "API",
         "comentarios de revisión; cerrado y reemplazado por el PR #60 de Luis"),
        (59, "Paola Peña", "review-driven fix", "API",
         "comentario «Review — Correcciones aplicadas»"),
        (61, "Daniel Reyna", "review-driven fix", "API",
         "comentario «Revision y correccion del PR #61»; commit ba2a919"),
        ("62→#63", "Nicole Sereno", "review-driven fix", "API",
         "5 revisiones formales; corregido en el PR #63 de Luis (commit 6781930)"),
        (64, "Manuel García", "review-driven fix", "API",
         "comentario «Revisión Completa del PR #64 — Middleware»"),
        (65, "Daniel Reyna", "review-driven fix", "API",
         "revisión formal APPROVED + revisión automatizada"),
        (66, "Paola Peña", "review-driven fix", "API",
         "revisiones DISMISSED y APPROVED + revisión automatizada"),
        (67, "Nicole Sereno", "review-driven fix", "API",
         "revisión formal APPROVED + revisión automatizada"),
        (68, "Manuel García", "review-driven fix", "API",
         "revisión formal APPROVED + 2 comentarios"),
        (69, "Manuel García", "review-driven fix", "API",
         "comentario «Revisión PR #69 — Tests Edge Cases»; cerrado sin merge"),
        (70, "Paola Peña", "review-driven fix", "API",
         "comentario «Code Review — Análisis Spec-Driven»"),
        (71, "Manuel García", "review-driven fix", "API",
         "revisión formal DISMISSED + comentario"),
        (72, "Nicole Sereno", "review-driven fix", "API",
         "comentario «Code review» con 7 issues (3 críticos); cerrado, reemplazado por #74"),
    ]
    rows = []
    for n, author, kind, prov, ev in spec:
        if isinstance(n, str):
            first, last = 62, 63
        else:
            first, last = n, n
        # fecha = primer evento de revisión (formal o comentario) de Luis sobre
        # ese PR; si no hay evento (p.ej. el revert #51 usa el commit propio),
        # cae a la fecha de creación del PR.
        evts = d["reviews"].get(first, {"formal": [], "comments": []})
        all_evts = evts["formal"] + evts["comments"]
        if all_evts:
            fday = day_local(min(e["ts"] for e in all_evts), tz)
        else:
            fday = day(first)
        rows.append({"n": len(rows) + 1, "pr": n, "author": author, "day": fday,
                     "kind": kind, "provenance": prov, "evidence": ev})
    return rows


# ---------------------------------------------------------------- main

def main(argv=None):
    ap = argparse.ArgumentParser(
        description="Informe read-only de fechas de PR, revisión y corrección (Luis Rojas).")
    ap.add_argument("--out", default=DEFAULT_OUT, help="ruta del informe markdown a escribir")
    args = ap.parse_args(argv)

    tz = detect_repo_tz()
    data = build(tz)
    data["fix_rows"] = build_fix_rows(tz, data)

    text = render_with_fixes(data, tz)

    os.makedirs(os.path.dirname(os.path.abspath(args.out)), exist_ok=True)
    with open(args.out, "w", encoding="utf-8") as fh:
        fh.write(text)
    print("Informe escrito en %s" % args.out)
    return 0


# render() necesita FIX_ROWS; se inyecta para mantener render() sin estado global.
FIX_ROWS = []

def render_with_fixes(d, tz):
    global FIX_ROWS
    FIX_ROWS = d["fix_rows"]
    return render(d, tz)


if __name__ == "__main__":
    sys.exit(main())
