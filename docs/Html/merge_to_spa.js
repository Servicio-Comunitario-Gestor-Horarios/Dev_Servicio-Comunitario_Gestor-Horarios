const fs = require('fs');
const path = require('path');

const filesToMerge = [
    { id: 'page-login', file: 'login.html' },
    { id: 'page-dashboard', file: 'dashboard.html' },
    { id: 'page-admin', file: 'Admin.html' },
    { id: 'page-docentes', file: 'Admin-Gestion-docente.html' },
    { id: 'page-aulas', file: 'Admin-gestion-aulas.html' },
    { id: 'page-asignaturas', file: 'Admin-gestion-asignatura.html' },
    { id: 'page-generacion', file: 'Admin-generacion-horarios.html' },
    { id: 'page-visualizar', file: 'Admin-visualizar-horarios.html' }
];

let finalHtml = `<!DOCTYPE html>
<html lang="es">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Prototipo Completo - Liceo Robert Serra</title>
    <link href="https://fonts.googleapis.com/css2?family=Inter:wght@300;400;500;600;700&display=swap" rel="stylesheet">
    <link rel="stylesheet" href="https://fonts.googleapis.com/css2?family=Material+Symbols+Outlined:opsz,wght,FILL,GRAD@24,300,0,0" />
    <style>
        .spa-page { display: none; width: 100vw; height: 100vh; overflow: hidden; }
        .spa-page.active { display: flex; }
`;

let allCss = new Set(); // to avoid some massive duplication, though not strictly necessary
let bodyContents = '';
let scripts = '';

filesToMerge.forEach((item, index) => {
    const filePath = path.join(__dirname, item.file);
    if (!fs.existsSync(filePath)) return;
    
    let content = fs.readFileSync(filePath, 'utf8');
    
    // Extract CSS
    const cssMatch = content.match(/<style>([\s\S]*?)<\/style>/);
    if (cssMatch) {
        finalHtml += `\n        /* CSS from ${item.file} */\n` + cssMatch[1] + '\n';
    }
    
    // Extract Body
    const bodyMatch = content.match(/<body>([\s\S]*?)<\/body>/);
    if (bodyMatch) {
        let bodyHtml = bodyMatch[1];
        
        // Convert hrefs to JS function calls
        // e.g. href="Admin-gestion-aulas.html" -> href="#" onclick="goToPage('page-aulas'); return false;"
        filesToMerge.forEach(target => {
            const regex = new RegExp(`href=["']${target.file}["']`, 'g');
            bodyHtml = bodyHtml.replace(regex, `href="#" onclick="goToPage('${target.id}'); return false;"`);
        });
        
        // Also fix the login button which is in a form maybe
        if (item.id === 'page-login') {
            bodyHtml = bodyHtml.replace(/onclick="window\.location\.href='Admin\.html'"/g, `onclick="goToPage('page-admin')"`);
        }

        // Isolate scripts from body to avoid running them multiple times or breaking
        const scriptRegex = /<script>([\s\S]*?)<\/script>/g;
        let match;
        while ((match = scriptRegex.exec(bodyHtml)) !== null) {
            scripts += `\n/* JS from ${item.file} */\n` + match[1];
        }
        bodyHtml = bodyHtml.replace(scriptRegex, '');
        
        // Add to body contents
        const activeClass = index === 0 ? 'active' : '';
        bodyContents += `\n    <!-- PAGE: ${item.file} -->\n`;
        bodyContents += `    <div id="${item.id}" class="spa-page ${activeClass}">\n`;
        bodyContents += bodyHtml;
        bodyContents += `\n    </div>\n`;
    }
});

finalHtml += `
    </style>
</head>
<body>
    ${bodyContents}
    
    <script>
        function goToPage(pageId) {
            document.querySelectorAll('.spa-page').forEach(page => {
                page.classList.remove('active');
            });
            document.getElementById(pageId).classList.add('active');
            
            // Special fix for dropdowns being left open
            document.querySelectorAll('.dropdown-panel').forEach(panel => {
                panel.classList.remove('show');
            });
        }
        
        ${scripts}
    </script>
</body>
</html>`;

fs.writeFileSync(path.join(__dirname, 'index.html'), finalHtml, 'utf8');
console.log('Successfully created Single Page Application in index.html');
