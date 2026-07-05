const fs = require('fs');

const htmlFiles = fs.readdirSync(__dirname).filter(f => f.endsWith('.html') && f !== 'login.html');

htmlFiles.forEach(file => {
    let content = fs.readFileSync(file, 'utf8');
    
    // For dashboard.html (Super Admin), include Gestión de Usuario. For others, don't.
    const isSuperAdmin = file.toLowerCase() === 'dashboard.html';

    const links = [
        { href: 'Admin.html', icon: 'home', text: 'Inicio', section: null }
    ];

    if (isSuperAdmin) {
        links.push({ isSection: true, text: 'Configuración' });
        links.push({ href: 'dashboard.html', icon: 'manage_accounts', text: 'Gestión de usuario', section: null });
    }

    links.push(
        { isSection: true, text: 'Académico' },
        { href: 'Admin-Gestion-docente.html', icon: 'school', text: 'Docentes', section: null },
        { href: 'Admin-gestion-aulas.html', icon: 'meeting_room', text: 'Aulas', section: null },
        { href: 'Admin-gestion-asignatura.html', icon: 'menu_book', text: 'Asignaturas', section: null },
        { isSection: true, text: 'Horarios' },
        { href: 'Admin-generacion-horarios.html', icon: 'calendar_clock', text: 'Generación de horario', section: null },
        { href: 'Admin-visualizar-horarios.html', icon: 'event_note', text: 'Visualización de horario', section: null }
    );

    let newNav = '<nav class="sidebar-nav">\n';
    
    links.forEach(link => {
        if (link.isSection) {
            newNav += `            <div class="nav-section">${link.text}</div>\n`;
        } else {
            const isActive = file.toLowerCase() === link.href.toLowerCase();
            const activeClass = isActive ? ' active' : '';
            newNav += `            <a href="${link.href}" class="nav-item${activeClass}">\n`;
            newNav += `                <span class="material-symbols-outlined">${link.icon}</span>\n`;
            newNav += `                ${link.text}\n`;
            newNav += `            </a>\n`;
        }
    });
    newNav += '        </nav>';
    
    const regex = /<nav class="sidebar-nav">[\s\S]*?<\/nav>/;
    if (regex.test(content)) {
        content = content.replace(regex, newNav);
        fs.writeFileSync(file, content, 'utf8');
        console.log('Updated nav in ' + file);
    }
});
