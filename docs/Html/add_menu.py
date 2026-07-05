import os
import glob
import re

html_files = glob.glob(r'c:\Users\paola_yl5l86j\OneDrive\Documentos\Html\*.html')

css_to_inject = """
        /* DROPDOWN PANEL STYLES */
        .menu-toggle { position: relative; }
        .dropdown-panel {
            position: absolute;
            top: 50px;
            left: 0;
            width: 220px;
            background: var(--bg-panel);
            border: 1px solid var(--border-light);
            border-radius: 8px;
            box-shadow: 0 10px 15px -3px rgba(0,0,0,0.1);
            display: none;
            flex-direction: column;
            z-index: 1000;
            cursor: default;
            text-align: left;
        }
        .dropdown-panel.show { display: flex; }
        .dropdown-header {
            padding: 15px; border-bottom: 1px solid var(--border-light);
            display: flex; flex-direction: column; gap: 4px;
        }
        .dropdown-header strong { font-size: 0.9rem; color: var(--text-dark); line-height: 1.2; }
        .dropdown-header span { font-size: 0.75rem; color: var(--text-muted); }
        .dropdown-item {
            padding: 12px 15px; display: flex; align-items: center; gap: 10px;
            color: var(--text-dark); text-decoration: none; font-size: 0.85rem; font-weight: 500;
            transition: background 0.2s;
        }
        .dropdown-item:hover { background: var(--hover-bg); color: var(--primary-color); }
        .dropdown-divider { height: 1px; background: var(--border-light); margin: 5px 0; }
        .text-danger { color: #dc2626 !important; }
        .text-danger:hover { background: #fef2f2 !important; color: #dc2626 !important; }
"""

html_menu_toggle_new = """<div class="menu-toggle" onclick="toggleProfileMenu(event)">
                <span class="material-symbols-outlined" style="font-size: 1.5rem;">menu</span>
                <div class="dropdown-panel" id="dropdownPanel" onclick="event.stopPropagation()">
                    <div class="dropdown-header">
                        <strong>Administrador</strong>
                        <span>admin@liceorobertserra.com</span>
                    </div>
                    <a href="#" class="dropdown-item">
                        <span class="material-symbols-outlined" style="font-size: 1.2rem;">person</span> Mi Perfil
                    </a>
                    <a href="#" class="dropdown-item">
                        <span class="material-symbols-outlined" style="font-size: 1.2rem;">settings</span> Configuración
                    </a>
                    <div class="dropdown-divider"></div>
                    <a href="login.html" class="dropdown-item text-danger">
                        <span class="material-symbols-outlined" style="font-size: 1.2rem;">logout</span> Cerrar Sesión
                    </a>
                </div>
            </div>"""

js_to_inject = """
    <script>
        function toggleProfileMenu(e) {
            e.stopPropagation();
            const panel = document.getElementById('dropdownPanel');
            if(panel) panel.classList.toggle('show');
        }
        window.addEventListener('click', () => {
            const panel = document.getElementById('dropdownPanel');
            if(panel) panel.classList.remove('show');
        });
    </script>
</body>
"""

for f in html_files:
    if 'login.html' in f: continue
    
    with open(f, 'r', encoding='utf-8') as file:
        content = file.read()
    
    modified = False
    
    if '</style>' in content and '/* DROPDOWN PANEL STYLES */' not in content:
        content = content.replace('</style>', css_to_inject + '\n    </style>')
        modified = True
        
    # Pattern to match the menu-toggle div block exactly
    pattern = r'<div class="menu-toggle">\s*<span class="material-symbols-outlined" style="font-size: 1\.5rem;">menu</span>\s*</div>'
    if re.search(pattern, content):
        content = re.sub(pattern, html_menu_toggle_new, content)
        modified = True
        
    if '</body>' in content and 'toggleProfileMenu' not in content:
        content = content.replace('</body>', js_to_inject)
        modified = True
        
    if modified:
        with open(f, 'w', encoding='utf-8') as file:
            file.write(content)
        print(f'Updated {f}')
