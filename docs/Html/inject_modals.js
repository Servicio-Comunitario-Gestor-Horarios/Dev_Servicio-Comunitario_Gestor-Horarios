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

const modalsHtml = `
    <!-- MODAL MI PERFIL -->
    <div class="modal-overlay" id="profileModal">
        <div class="modal" style="max-width: 400px;">
            <div class="modal-header">
                <div class="modal-title">
                    <span class="material-symbols-outlined" style="color: var(--primary-color);">person</span>
                    Mi Perfil
                </div>
                <button class="close-modal" onclick="closeModal('profileModal')"><span class="material-symbols-outlined">close</span></button>
            </div>
            <div class="modal-body" style="text-align: center;">
                <div class="user-avatar-text" style="width: 80px; height: 80px; font-size: 2rem; margin: 0 auto 15px auto;">
                    A
                </div>
                <h3 style="margin-bottom: 5px; color: var(--text-dark);">Administrador</h3>
                <p style="color: var(--text-muted); font-size: 0.9rem; margin-bottom: 20px;">admin@liceorobertserra.com</p>
                
                <div style="text-align: left; margin-bottom: 15px;">
                    <label style="display: block; font-size: 0.85rem; font-weight: 600; margin-bottom: 6px;">Cambiar Contraseña</label>
                    <input type="password" class="form-control" placeholder="Nueva contraseña" style="margin-bottom: 10px;">
                    <input type="password" class="form-control" placeholder="Confirmar contraseña">
                </div>
            </div>
            <div class="modal-footer">
                <button type="button" class="btn-outline" onclick="closeModal('profileModal')">Cerrar</button>
                <button type="button" class="btn-primary" onclick="closeModal('profileModal'); alert('Perfil actualizado con éxito');">Guardar Cambios</button>
            </div>
        </div>
    </div>

    <!-- MODAL CONFIGURACIÓN -->
    <div class="modal-overlay" id="configModal">
        <div class="modal" style="max-width: 450px;">
            <div class="modal-header">
                <div class="modal-title">
                    <span class="material-symbols-outlined" style="color: var(--primary-color);">settings</span>
                    Configuración del Sistema
                </div>
                <button class="close-modal" onclick="closeModal('configModal')"><span class="material-symbols-outlined">close</span></button>
            </div>
            <div class="modal-body">
                <div style="display: flex; justify-content: space-between; align-items: center; padding: 12px 0; border-bottom: 1px solid var(--border-light);">
                    <div>
                        <strong style="display: block; font-size: 0.95rem;">Notificaciones por Email</strong>
                        <span style="font-size: 0.8rem; color: var(--text-muted);">Recibir avisos de cruces de horarios</span>
                    </div>
                    <label style="position: relative; display: inline-block; width: 44px; height: 24px;">
                        <input type="checkbox" checked style="opacity: 0; width: 0; height: 0;">
                        <span style="position: absolute; cursor: pointer; top: 0; left: 0; right: 0; bottom: 0; background-color: var(--primary-color); border-radius: 24px; transition: .4s;">
                            <span style="position: absolute; height: 18px; width: 18px; left: 22px; bottom: 3px; background-color: white; border-radius: 50%; transition: .4s;"></span>
                        </span>
                    </label>
                </div>
                
                <div style="display: flex; justify-content: space-between; align-items: center; padding: 12px 0; border-bottom: 1px solid var(--border-light);">
                    <div>
                        <strong style="display: block; font-size: 0.95rem;">Modo Oscuro</strong>
                        <span style="font-size: 0.8rem; color: var(--text-muted);">Apariencia de la interfaz</span>
                    </div>
                    <label style="position: relative; display: inline-block; width: 44px; height: 24px;">
                        <input type="checkbox" style="opacity: 0; width: 0; height: 0;">
                        <span style="position: absolute; cursor: pointer; top: 0; left: 0; right: 0; bottom: 0; background-color: #cbd5e1; border-radius: 24px; transition: .4s;">
                            <span style="position: absolute; height: 18px; width: 18px; left: 4px; bottom: 3px; background-color: white; border-radius: 50%; transition: .4s;"></span>
                        </span>
                    </label>
                </div>
                
                <div style="display: flex; justify-content: space-between; align-items: center; padding: 12px 0;">
                    <div>
                        <strong style="display: block; font-size: 0.95rem;">Respaldo Automático</strong>
                        <span style="font-size: 0.8rem; color: var(--text-muted);">Sincronizar base de datos diaria</span>
                    </div>
                    <label style="position: relative; display: inline-block; width: 44px; height: 24px;">
                        <input type="checkbox" checked style="opacity: 0; width: 0; height: 0;">
                        <span style="position: absolute; cursor: pointer; top: 0; left: 0; right: 0; bottom: 0; background-color: var(--primary-color); border-radius: 24px; transition: .4s;">
                            <span style="position: absolute; height: 18px; width: 18px; left: 22px; bottom: 3px; background-color: white; border-radius: 50%; transition: .4s;"></span>
                        </span>
                    </label>
                </div>
            </div>
            <div class="modal-footer">
                <button type="button" class="btn-primary" onclick="closeModal('configModal')">Aceptar</button>
            </div>
        </div>
    </div>
`;

filesToUpdate.forEach(file => {
    const filePath = path.join(__dirname, file);
    if (!fs.existsSync(filePath)) return;
    
    let content = fs.readFileSync(filePath, 'utf8');
    
    // Update hrefs to open modals
    content = content.replace(/<a href="#" class="dropdown-item">\s*<span class="material-symbols-outlined"[^>]*>person<\/span>\s*Mi Perfil\s*<\/a>/g, `<a href="#" class="dropdown-item" onclick="closeDropdown(); openModal('profileModal'); return false;">\n                        <span class="material-symbols-outlined" style="font-size: 1.2rem;">person</span> Mi Perfil\n                    </a>`);
    
    content = content.replace(/<a href="#" class="dropdown-item">\s*<span class="material-symbols-outlined"[^>]*>settings<\/span>\s*Configuración\s*<\/a>/g, `<a href="#" class="dropdown-item" onclick="closeDropdown(); openModal('configModal'); return false;">\n                        <span class="material-symbols-outlined" style="font-size: 1.2rem;">settings</span> Configuración\n                    </a>`);

    // Ensure we have closeDropdown function
    if (!content.includes('function closeDropdown()')) {
        content = content.replace('// Profile Menu Toggle', `function closeDropdown() { const p = document.getElementById('dropdownPanel'); if(p) p.classList.remove('show'); }\n        // Profile Menu Toggle`);
    }

    // Insert Modals before script if they don't exist
    if (!content.includes('id="profileModal"')) {
        content = content.replace('<script>', modalsHtml + '\n    <script>');
    }
    
    fs.writeFileSync(filePath, content, 'utf8');
    console.log('Injected modals into ' + file);
});
