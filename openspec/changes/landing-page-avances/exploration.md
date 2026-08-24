## Exploration: landing-page-avances

### Current State

No landing page or public-facing "avances" page exists today. Web-adjacent artifacts available as raw material:
- `docs/Html/index.html` — HTML prototype of the app UI (inline iframes, Google Fonts Inter, Material Symbols Outlined, palette: primary #1e3a8a, bg #f8fafc, card #ffffff, text #0f172a, muted #64748b, border #e2e8f0, input-bg #f1f5f9)
- `docs/informes/informe_sprint_institucional_semana_{2,3,4,5}.md` — advisor-oriented weekly progress reports with mermaid diagrams and per-person "¿Qué significa para el Liceo?" narratives

Project state (content sources for the page):
- Phases: Cimientos 100%, MVP (Sprints 3-5) 100% → 23/73 tasks (~32% of project total)
- Solver CP-SAT fully functional since Sprint 5: 8 hard constraints (R1-R8), 13 pre-solver validations (V1-V13), 10 unit tests + benchmark, ~1,440 LOC engine (`docs/solver/Informe-Motor-Solver.md`)
- Export JSON/CSV, SQLite 7 tables, single binary dual-mode (--backend)
- Code metrics: 8,151 LOC, 42 .cpp, 41 headers, 23 test files
- Git: 71 commits on develop (152 all-branch); per person: Luis 111, Manuel 15, Paola 13, Daniel 7, Nicole 6
- Sprint 5 deliveries (20-26 Jul): 6 PRs merged — #75 (+2847), #74 (+892), #73 (+1245), #71 (+678), #70 (+1567), #68 (+312)
- Team: Luis Rojas (Tech Lead/Backend), Daniel Reyna (Tech Lead/Frontend), Paola Peña (Frontend), Nicole Sereno (Backend), Manuel García (Middleware/QA)
- Beneficiary: Liceo Nacional Robert Serra — 41 direct beneficiaries (11H/30M), 933 students indirect (974 total); UNEG Servicio Comunitario; advisor Prof. Bárbara Marquina; community advisor Diego Calzadilla
- Roadmap: Planilla 3 (docs/propuestas/Anteproyecto.pdf → 13 weeks, 22 Jun - 11 Nov 2026, 136h/person, vacation 1 Ago - 23 Sep); GitHub milestones: MVP Base due 2026-08-31 (5/24 open), Beta Sprints 6-9 due 2026-12-31, Release 1.0 due 2027-04-30
- Current sprint 6 (27-31 Jul): #39 soft constraints, #37 tuning, #41 performance, #36 admin/users, #38 config screen, #40 integration tests; ServicioProfesor/ServicioDisponibilidad needed (Luis)
- GitHub Pages NOT enabled (gh pages API 404); org: Servicio-Comunitario-Gestor-Horarios
- Context driver: `docs/reunion.md` — team needs "herramientas para ponerla al día de los avances" for the academic advisor; advisor does not reliably read weekly reports

### Affected Areas

- No existing files are modified — new content only
- `docs/contribuciones/*.md` (untracked in git), `docs/informes/*.md`, `docs/propuestas/Propuesta Producto Mínimo Viable.md`, `docs/Html/index.html` — source material
- `README.md` — stack table + project structure (tech section reference)
- `.github/workflows/` — candidate for Pages deployment workflow (auto-move-project.yml, board-screenshot.yml, pr-project-status.yml exist)
- `config/Solver.json` — institutional parameters (turno Mañana 07:00-12:30 / Tarde 12:35-18:10, recesos 08:20-08:30 / 14:00-14:10) for a "cómo funciona" section

### Approaches

1. **Static single-file HTML landing page in repo** (`docs/landing-page/index.html`)
   - Self-contained (inline CSS/JS, no build step), reuses the docs/Html visual language
   - Sections: proyecto / equipo / avances timeline (semanas 2-5) / métricas / roadmap (Planilla 3 + milestones) / contacto
   - Pros: zero infrastructure, works via file:// or static hosting, committable via PR, matches existing prototype design, directly answers the reunion.md need
   - Cons: no shareable URL out of the box, manual per-sprint updates
   - Effort: Low-Medium

2. **GitHub Pages site** (enable Pages on repo + deploy via Actions)
   - Public URL: `https://Servicio-Comunitario-Gestor-Horarios.github.io/Gestor-Horarios/`
   - Pros: real shareable URL for advisor/institution, automatable deployment, professional presentation
   - Cons: requires org admin rights (Pages currently disabled — org permission unconfirmed), public content exposure decision, extra CI surface
   - Effort: Medium

3. **Markdown progress page** (`docs/avances.md` + README link)
   - Pros: lowest effort, renders natively on GitHub, trivial to update each sprint
   - Cons: not a "landing page" — no visual identity, weaker impact for the advisor
   - Effort: Low

### Recommendation

Option 1 (static HTML at `docs/landing-page/index.html`) as the default; Option 2 (GitHub Pages) as a follow-up if the team confirms org admin can enable Pages. Rationale: zero-risk, immediately committable, reuses the existing design system, and directly addresses the reunion.md communication gap — the advisor can open the file directly or the team can present it in meetings. Option 3 does not meet the "landing page" bar.

### Risks

- GitHub Pages requires org admin permissions — must be confirmed before committing to Option 2
- Sprint numbering drift: institutional reports (semana 5 = 20-26 Jul) vs planning docs (issues-detalladas: Sprint 6 = 20-26 Jul) — the page must use ONE consistent timeline (recommend Planilla 3 weeks)
- Metrics (commits, LOC, tests, PRs) go stale per sprint — include a "generado el" date and a per-sprint update ritual
- `docs/contribuciones/` and `docs/reunion.md` are untracked in git — landing page source material must be committed
- Vacation 1 Ago - 23 Sep (liceo cerrado) affects announcement/publishing timing
- Content is Spanish — keep it in Spanish for advisor/institution audience

### Ready for Proposal

Yes. Orchestrator should confirm with the user: (a) target audience (advisor-only vs institution-wide), (b) is GitHub Pages enablement possible (org admin), (c) static single-file vs Pages deployment.
