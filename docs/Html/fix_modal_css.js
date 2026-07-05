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

const modalCss = `
        /* --- MODAL STYLES --- */
        .modal-overlay {
            position: fixed;
            top: 0; left: 0; right: 0; bottom: 0;
            background-color: rgba(15, 23, 42, 0.5);
            backdrop-filter: blur(4px);
            display: flex; align-items: center; justify-content: center;
            z-index: 9999;
            opacity: 0; pointer-events: none;
            transition: opacity 0.3s ease;
        }
        .modal-overlay.active {
            opacity: 1; pointer-events: auto;
        }
        .modal {
            background-color: var(--bg-panel, #ffffff);
            width: 100%; max-width: 600px;
            border-radius: 16px;
            box-shadow: 0 20px 25px -5px rgba(0,0,0,0.1);
            transform: translateY(20px);
            transition: transform 0.3s ease;
            display: flex; flex-direction: column;
            overflow: hidden; border: 1px solid var(--border-light, #e2e8f0);
        }
        .modal-overlay.active .modal {
            transform: translateY(0);
        }
        .modal-header {
            padding: 20px 24px; border-bottom: 1px solid var(--border-light, #e2e8f0);
            display: flex; justify-content: space-between; align-items: center;
            background-color: #f8fafc;
        }
        .modal-title {
            font-size: 1.15rem; font-weight: 700; color: var(--text-dark, #0f172a);
            display: flex; align-items: center; gap: 8px;
        }
        .close-modal {
            background: transparent; border: none; color: var(--text-muted, #64748b);
            cursor: pointer; border-radius: 6px; width: 32px; height: 32px;
            display: flex; align-items: center; justify-content: center;
            transition: all 0.2s;
        }
        .close-modal:hover { background-color: #e2e8f0; color: var(--text-dark, #0f172a); }
        .modal-body { padding: 24px; max-height: 70vh; overflow-y: auto; background-color: var(--bg-panel, #ffffff); }
        .modal-footer {
            padding: 16px 24px; border-top: 1px solid var(--border-light, #e2e8f0);
            display: flex; justify-content: flex-end; gap: 12px; background-color: #f8fafc;
        }
`;

filesToUpdate.forEach(file => {
    const filePath = path.join(__dirname, file);
    if (!fs.existsSync(filePath)) return;
    
    let content = fs.readFileSync(filePath, 'utf8');
    
    if (!content.includes('.modal-overlay {') && !content.includes('.modal-overlay{')) {
        content = content.replace('</style>', modalCss + '\n    </style>');
        fs.writeFileSync(filePath, content, 'utf8');
        console.log('Injected Modal CSS into ' + file);
    } else {
        // Even if it exists, let's make sure the z-index is high enough
        content = content.replace(/z-index:\s*100;/, 'z-index: 9999;');
        fs.writeFileSync(filePath, content, 'utf8');
        console.log('Updated z-index in ' + file);
    }
});
