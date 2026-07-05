const fs = require('fs');
const path = require('path');

const filesToMerge = [
    'login.html',
    'dashboard.html',
    'Admin.html',
    'Admin-Gestion-docente.html',
    'Admin-gestion-aulas.html',
    'Admin-gestion-asignatura.html',
    'Admin-generacion-horarios.html',
    'Admin-visualizar-horarios.html'
];

function escapeHtml(str) {
    return str
        .replace(/&/g, '&amp;')
        .replace(/</g, '&lt;')
        .replace(/>/g, '&gt;')
        .replace(/"/g, '&quot;')
        .replace(/'/g, '&#39;');
}

const interceptScript = `
<script>
// Intercept all clicks on links
document.addEventListener('click', function(e) {
    const a = e.target.closest('a');
    if (a && a.getAttribute('href')) {
        let href = a.getAttribute('href');
        if (href.endsWith('.html')) {
            e.preventDefault();
            window.parent.postMessage({ type: 'NAVIGATE', to: href }, '*');
        }
    }
});
</script>
`;

let iframesHtml = '';

filesToMerge.forEach((file, index) => {
    const filePath = path.join(__dirname, file);
    if (!fs.existsSync(filePath)) return;
    
    let content = fs.readFileSync(filePath, 'utf8');
    
    // Fix the login button's window.location.href
    content = content.replace(/window\.location\.href\s*=\s*['"]([^'"]+\.html)['"]/g, "window.parent.postMessage({ type: 'NAVIGATE', to: '$1' }, '*')");

    // Inject the click interceptor right before </body>
    content = content.replace('</body>', interceptScript + '\n</body>');

    const escapedContent = escapeHtml(content);
    const activeClass = index === 0 ? 'active' : '';
    
    iframesHtml += `    <iframe id="frame-${file}" class="${activeClass}" srcdoc="${escapedContent}"></iframe>\n`;
});

const finalHtml = `<!DOCTYPE html>
<html lang="es">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Prototipo Completo - Liceo Robert Serra</title>
    <style>
        body, html { margin: 0; padding: 0; width: 100vw; height: 100vh; overflow: hidden; background-color: #f8fafc; }
        iframe { border: none; width: 100%; height: 100%; display: none; }
        iframe.active { display: block; }
    </style>
</head>
<body>
${iframesHtml}
    <script>
        window.addEventListener('message', function(e) {
            if (e.data && e.data.type === 'NAVIGATE') {
                const targetFrame = document.getElementById('frame-' + e.data.to);
                if (targetFrame) {
                    document.querySelectorAll('iframe').forEach(f => f.classList.remove('active'));
                    targetFrame.classList.add('active');
                } else {
                    console.error('Page not found:', e.data.to);
                }
            }
        });
    </script>
</body>
</html>`;

fs.writeFileSync(path.join(__dirname, 'index.html'), finalHtml, 'utf8');
console.log('Successfully created Isolated Iframe SPA in index.html');
