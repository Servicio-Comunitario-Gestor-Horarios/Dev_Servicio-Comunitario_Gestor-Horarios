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

const newConfigModalHtml = `
    <!-- MODAL CONFIGURACIÓN -->
    <div class="modal-overlay" id="configModal">
        <div class="modal" style="max-width: 600px;">
            <div class="modal-header">
                <div class="modal-title">
                    <span class="material-symbols-outlined" style="color: var(--primary-color);">settings</span>
                    Configuración del Sistema
                </div>
                <button class="close-modal" onclick="closeSystemModal('configModal')"><span class="material-symbols-outlined">close</span></button>
            </div>
            <div class="modal-body" style="display: grid; grid-template-columns: 1fr 1fr; gap: 30px;">
                <!-- General Settings Column -->
                <div>
                    <h4 style="font-size: 0.95rem; color: var(--primary-color); margin-bottom: 16px; border-bottom: 2px solid var(--border-light); padding-bottom: 6px;">Preferencias Generales</h4>
                    
                    <div style="display: flex; justify-content: space-between; align-items: center; margin-bottom: 18px;">
                        <div>
                            <strong style="display: block; font-size: 0.9rem;">Modo Oscuro</strong>
                            <span style="font-size: 0.75rem; color: var(--text-muted);">Apariencia de la interfaz</span>
                        </div>
                        <label style="position: relative; display: inline-block; width: 40px; height: 22px;">
                            <input type="checkbox" style="opacity: 0; width: 0; height: 0;">
                            <span style="position: absolute; cursor: pointer; top: 0; left: 0; right: 0; bottom: 0; background-color: #cbd5e1; border-radius: 20px; transition: .4s;">
                                <span style="position: absolute; height: 16px; width: 16px; left: 3px; bottom: 3px; background-color: white; border-radius: 50%; transition: .4s;"></span>
                            </span>
                        </label>
                    </div>

                    <div style="display: flex; justify-content: space-between; align-items: center; margin-bottom: 18px;">
                        <div>
                            <strong style="display: block; font-size: 0.9rem;">Notificaciones por Email</strong>
                            <span style="font-size: 0.75rem; color: var(--text-muted);">Avisos de cruces de horarios</span>
                        </div>
                        <label style="position: relative; display: inline-block; width: 40px; height: 22px;">
                            <input type="checkbox" checked style="opacity: 0; width: 0; height: 0;">
                            <span style="position: absolute; cursor: pointer; top: 0; left: 0; right: 0; bottom: 0; background-color: var(--primary-color); border-radius: 20px; transition: .4s;">
                                <span style="position: absolute; height: 16px; width: 16px; left: 21px; bottom: 3px; background-color: white; border-radius: 50%; transition: .4s;"></span>
                            </span>
                        </label>
                    </div>

                    <div style="display: flex; justify-content: space-between; align-items: center; margin-bottom: 18px;">
                        <div>
                            <strong style="display: block; font-size: 0.9rem;">Respaldo Automático</strong>
                            <span style="font-size: 0.75rem; color: var(--text-muted);">Copia de seguridad diaria</span>
                        </div>
                        <label style="position: relative; display: inline-block; width: 40px; height: 22px;">
                            <input type="checkbox" checked style="opacity: 0; width: 0; height: 0;">
                            <span style="position: absolute; cursor: pointer; top: 0; left: 0; right: 0; bottom: 0; background-color: var(--primary-color); border-radius: 20px; transition: .4s;">
                                <span style="position: absolute; height: 16px; width: 16px; left: 21px; bottom: 3px; background-color: white; border-radius: 50%; transition: .4s;"></span>
                            </span>
                        </label>
                    </div>
                </div>

                <!-- Academic Settings Column -->
                <div>
                    <h4 style="font-size: 0.95rem; color: var(--primary-color); margin-bottom: 16px; border-bottom: 2px solid var(--border-light); padding-bottom: 6px;">Parámetros Académicos</h4>
                    
                    <div style="margin-bottom: 16px;">
                        <strong style="display: block; font-size: 0.9rem; margin-bottom: 6px;">Duración de Bloque</strong>
                        <select class="form-control" style="font-size: 0.85rem; padding: 8px 12px; background-color: #f8fafc;">
                            <option>45 Minutos (Estándar)</option>
                            <option>60 Minutos (Largo)</option>
                            <option>90 Minutos (Bloque Doble)</option>
                        </select>
                    </div>
                    
                    <div style="margin-bottom: 16px;">
                        <strong style="display: block; font-size: 0.9rem; margin-bottom: 6px;">Jornada Laboral Diaria</strong>
                        <div style="display: flex; gap: 8px;">
                            <input type="time" class="form-control" value="07:00" style="font-size: 0.85rem; padding: 8px;">
                            <span style="align-self: center; font-weight: bold; color: var(--text-muted);">-</span>
                            <input type="time" class="form-control" value="17:00" style="font-size: 0.85rem; padding: 8px;">
                        </div>
                    </div>

                    <div style="display: flex; justify-content: space-between; align-items: center; margin-top: 22px;">
                        <div>
                            <strong style="display: block; font-size: 0.9rem;">Evitar Sobrecarga</strong>
                            <span style="font-size: 0.75rem; color: var(--text-muted);">No exceder 8h continuas</span>
                        </div>
                        <label style="position: relative; display: inline-block; width: 40px; height: 22px;">
                            <input type="checkbox" checked style="opacity: 0; width: 0; height: 0;">
                            <span style="position: absolute; cursor: pointer; top: 0; left: 0; right: 0; bottom: 0; background-color: var(--primary-color); border-radius: 20px; transition: .4s;">
                                <span style="position: absolute; height: 16px; width: 16px; left: 21px; bottom: 3px; background-color: white; border-radius: 50%; transition: .4s;"></span>
                            </span>
                        </label>
                    </div>
                </div>
            </div>
            <div class="modal-footer">
                <button type="button" class="btn-outline" onclick="closeSystemModal('configModal')">Descartar</button>
                <button type="button" class="btn-primary" onclick="closeSystemModal('configModal'); alert('Ajustes del sistema actualizados satisfactoriamente.');">Guardar Configuración</button>
            </div>
        </div>
    </div>`;

filesToUpdate.forEach(file => {
    const filePath = path.join(__dirname, file);
    if (!fs.existsSync(filePath)) return;
    
    let content = fs.readFileSync(filePath, 'utf8');
    
    // Replace the old config modal
    const regex = /<!-- MODAL CONFIGURACIÓN -->[\s\S]*?(?=<!--|\n\s*<script>|\n\s*<\/body>)/;
    if (regex.test(content)) {
        content = content.replace(regex, newConfigModalHtml + '\n');
        fs.writeFileSync(filePath, content, 'utf8');
        console.log('Updated config modal in ' + file);
    }
});
