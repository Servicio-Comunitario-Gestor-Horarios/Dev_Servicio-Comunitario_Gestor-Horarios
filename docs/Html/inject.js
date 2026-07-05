const fs = require('fs');

const files = fs.readdirSync(__dirname).filter(f => f.endsWith('.html') && f !== 'login.html');

const cssToInject = `
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
`;

const htmlMenuToggleNew = `<div class="menu-toggle" onclick="toggleProfileMenu(event)">
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
            </div>`;

const jsToInject = `
    <script>
        // Profile Menu Toggle
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
</body>`;

files.forEach(f => {
    let content = fs.readFileSync(f, 'utf8');
    let modified = false;

    if (content.includes('</style>') && !content.includes('/* DROPDOWN PANEL STYLES */')) {
        content = content.replace('</style>', cssToInject + '    </style>');
        modified = true;
    }

    const regex = /<div class="menu-toggle">\s*<span class="material-symbols-outlined" style="font-size: 1\.5rem;">menu<\/span>\s*<\/div>/g;
    if (regex.test(content)) {
        content = content.replace(regex, htmlMenuToggleNew);
        modified = true;
    }

    if (content.includes('</body>') && !content.includes('toggleProfileMenu(e)')) {
        content = content.replace('</body>', jsToInject);
        modified = true;
    }

    if (modified) {
        fs.writeFileSync(f, content, 'utf8');
        console.log('Updated ' + f);
    }
});
