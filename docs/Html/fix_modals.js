const fs = require('fs');
const path = require('path');

const filesToUpdate = [
    'dashboard.html',
    'Admin.html',
    'Admin-Gestion-docente.html',
    'Admin-gestion-aulas.html',
    'Admin-gestion-asignatura.html',
    'Admin-generacion-horarios.html',
    'Admin-visualizar-horarios.html'
];

const jsFix = `
    <script>
        function openSystemModal(id) { 
            const m = document.getElementById(id);
            if(m) m.classList.add('active'); 
        }
        function closeSystemModal(id) { 
            const m = document.getElementById(id);
            if(m) m.classList.remove('active'); 
        }
    </script>
`;

filesToUpdate.forEach(file => {
    const filePath = path.join(__dirname, file);
    if (!fs.existsSync(filePath)) return;
    
    let content = fs.readFileSync(filePath, 'utf8');
    
    // Change onclick for opening
    content = content.replace(/openModal\('profileModal'\)/g, "openSystemModal('profileModal')");
    content = content.replace(/openModal\('configModal'\)/g, "openSystemModal('configModal')");
    
    // Change onclick for closing inside the injected modals
    content = content.replace(/closeModal\('profileModal'\)/g, "closeSystemModal('profileModal')");
    content = content.replace(/closeModal\('configModal'\)/g, "closeSystemModal('configModal')");
    
    // Inject the functions before the closing body tag
    if (!content.includes('function openSystemModal(id)')) {
        content = content.replace('</body>', jsFix + '\n</body>');
    }
    
    fs.writeFileSync(filePath, content, 'utf8');
    console.log('Fixed modals in ' + file);
});
