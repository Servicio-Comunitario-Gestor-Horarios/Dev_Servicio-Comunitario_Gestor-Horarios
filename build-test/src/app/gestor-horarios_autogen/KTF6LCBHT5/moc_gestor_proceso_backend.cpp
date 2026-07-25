/****************************************************************************
** Meta object code from reading C++ file 'gestor_proceso_backend.hpp'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.4.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../../../src/app/include/app/gestor_proceso_backend.hpp"
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'gestor_proceso_backend.hpp' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 68
#error "This file was generated using the moc from 6.4.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

#ifndef Q_CONSTINIT
#define Q_CONSTINIT
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
namespace {
struct qt_meta_stringdata_GestorProcesoBackend_t {
    uint offsetsAndSizes[32];
    char stringdata0[21];
    char stringdata1[13];
    char stringdata2[1];
    char stringdata3[17];
    char stringdata4[19];
    char stringdata5[8];
    char stringdata6[16];
    char stringdata7[17];
    char stringdata8[19];
    char stringdata9[7];
    char stringdata10[21];
    char stringdata11[7];
    char stringdata12[22];
    char stringdata13[23];
    char stringdata14[6];
    char stringdata15[25];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(sizeof(qt_meta_stringdata_GestorProcesoBackend_t::offsetsAndSizes) + ofs), len 
Q_CONSTINIT static const qt_meta_stringdata_GestorProcesoBackend_t qt_meta_stringdata_GestorProcesoBackend = {
    {
        QT_MOC_LITERAL(0, 20),  // "GestorProcesoBackend"
        QT_MOC_LITERAL(21, 12),  // "backendListo"
        QT_MOC_LITERAL(34, 0),  // ""
        QT_MOC_LITERAL(35, 16),  // "backendColapsado"
        QT_MOC_LITERAL(52, 18),  // "backendReiniciando"
        QT_MOC_LITERAL(71, 7),  // "intento"
        QT_MOC_LITERAL(79, 15),  // "backendDetenido"
        QT_MOC_LITERAL(95, 16),  // "alIniciarProceso"
        QT_MOC_LITERAL(112, 18),  // "alFinalizarProceso"
        QT_MOC_LITERAL(131, 6),  // "codigo"
        QT_MOC_LITERAL(138, 20),  // "QProcess::ExitStatus"
        QT_MOC_LITERAL(159, 6),  // "estado"
        QT_MOC_LITERAL(166, 21),  // "alOcurrirErrorProceso"
        QT_MOC_LITERAL(188, 22),  // "QProcess::ProcessError"
        QT_MOC_LITERAL(211, 5),  // "error"
        QT_MOC_LITERAL(217, 24)   // "alExpiracionVerificacion"
    },
    "GestorProcesoBackend",
    "backendListo",
    "",
    "backendColapsado",
    "backendReiniciando",
    "intento",
    "backendDetenido",
    "alIniciarProceso",
    "alFinalizarProceso",
    "codigo",
    "QProcess::ExitStatus",
    "estado",
    "alOcurrirErrorProceso",
    "QProcess::ProcessError",
    "error",
    "alExpiracionVerificacion"
};
#undef QT_MOC_LITERAL
} // unnamed namespace

Q_CONSTINIT static const uint qt_meta_data_GestorProcesoBackend[] = {

 // content:
      10,       // revision
       0,       // classname
       0,    0, // classinfo
       8,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       4,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    0,   62,    2, 0x06,    1 /* Public */,
       3,    0,   63,    2, 0x06,    2 /* Public */,
       4,    1,   64,    2, 0x06,    3 /* Public */,
       6,    0,   67,    2, 0x06,    5 /* Public */,

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
       7,    0,   68,    2, 0x08,    6 /* Private */,
       8,    2,   69,    2, 0x08,    7 /* Private */,
      12,    1,   74,    2, 0x08,   10 /* Private */,
      15,    0,   77,    2, 0x08,   12 /* Private */,

 // signals: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Int,    5,
    QMetaType::Void,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void, QMetaType::Int, 0x80000000 | 10,    9,   11,
    QMetaType::Void, 0x80000000 | 13,   14,
    QMetaType::Void,

       0        // eod
};

Q_CONSTINIT const QMetaObject GestorProcesoBackend::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_meta_stringdata_GestorProcesoBackend.offsetsAndSizes,
    qt_meta_data_GestorProcesoBackend,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_stringdata_GestorProcesoBackend_t,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<GestorProcesoBackend, std::true_type>,
        // method 'backendListo'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'backendColapsado'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'backendReiniciando'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        // method 'backendDetenido'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'alIniciarProceso'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'alFinalizarProceso'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        QtPrivate::TypeAndForceComplete<QProcess::ExitStatus, std::false_type>,
        // method 'alOcurrirErrorProceso'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<QProcess::ProcessError, std::false_type>,
        // method 'alExpiracionVerificacion'
        QtPrivate::TypeAndForceComplete<void, std::false_type>
    >,
    nullptr
} };

void GestorProcesoBackend::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<GestorProcesoBackend *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->backendListo(); break;
        case 1: _t->backendColapsado(); break;
        case 2: _t->backendReiniciando((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        case 3: _t->backendDetenido(); break;
        case 4: _t->alIniciarProceso(); break;
        case 5: _t->alFinalizarProceso((*reinterpret_cast< std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QProcess::ExitStatus>>(_a[2]))); break;
        case 6: _t->alOcurrirErrorProceso((*reinterpret_cast< std::add_pointer_t<QProcess::ProcessError>>(_a[1]))); break;
        case 7: _t->alExpiracionVerificacion(); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (GestorProcesoBackend::*)();
            if (_t _q_method = &GestorProcesoBackend::backendListo; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (GestorProcesoBackend::*)();
            if (_t _q_method = &GestorProcesoBackend::backendColapsado; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (GestorProcesoBackend::*)(int );
            if (_t _q_method = &GestorProcesoBackend::backendReiniciando; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 2;
                return;
            }
        }
        {
            using _t = void (GestorProcesoBackend::*)();
            if (_t _q_method = &GestorProcesoBackend::backendDetenido; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 3;
                return;
            }
        }
    }
}

const QMetaObject *GestorProcesoBackend::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *GestorProcesoBackend::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_GestorProcesoBackend.stringdata0))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int GestorProcesoBackend::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 8)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 8;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 8)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 8;
    }
    return _id;
}

// SIGNAL 0
void GestorProcesoBackend::backendListo()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void GestorProcesoBackend::backendColapsado()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void GestorProcesoBackend::backendReiniciando(int _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}

// SIGNAL 3
void GestorProcesoBackend::backendDetenido()
{
    QMetaObject::activate(this, &staticMetaObject, 3, nullptr);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
