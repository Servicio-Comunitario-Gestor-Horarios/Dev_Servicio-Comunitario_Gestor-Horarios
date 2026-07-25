/****************************************************************************
** Meta object code from reading C++ file 'internalclient.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.4.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../../../src/middleware/include/middleware/internalclient.h"
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'internalclient.h' doesn't include <QObject>."
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
struct qt_meta_stringdata_InternalClient_t {
    uint offsetsAndSizes[30];
    char stringdata0[15];
    char stringdata1[28];
    char stringdata2[1];
    char stringdata3[8];
    char stringdata4[18];
    char stringdata5[10];
    char stringdata6[12];
    char stringdata7[12];
    char stringdata8[16];
    char stringdata9[31];
    char stringdata10[6];
    char stringdata11[16];
    char stringdata12[16];
    char stringdata13[3];
    char stringdata14[8];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(sizeof(qt_meta_stringdata_InternalClient_t::offsetsAndSizes) + ofs), len 
Q_CONSTINIT static const qt_meta_stringdata_InternalClient_t qt_meta_stringdata_InternalClient = {
    {
        QT_MOC_LITERAL(0, 14),  // "InternalClient"
        QT_MOC_LITERAL(15, 27),  // "healthCheckResponseReceived"
        QT_MOC_LITERAL(43, 0),  // ""
        QT_MOC_LITERAL(44, 7),  // "exitoso"
        QT_MOC_LITERAL(52, 17),  // "respuestaRecibida"
        QT_MOC_LITERAL(70, 9),  // "respuesta"
        QT_MOC_LITERAL(80, 11),  // "onConnected"
        QT_MOC_LITERAL(92, 11),  // "onReadyRead"
        QT_MOC_LITERAL(104, 15),  // "onErrorOccurred"
        QT_MOC_LITERAL(120, 30),  // "QLocalSocket::LocalSocketError"
        QT_MOC_LITERAL(151, 5),  // "error"
        QT_MOC_LITERAL(157, 15),  // "sendHealthCheck"
        QT_MOC_LITERAL(173, 15),  // "enviarSolicitud"
        QT_MOC_LITERAL(189, 2),  // "op"
        QT_MOC_LITERAL(192, 7)   // "payload"
    },
    "InternalClient",
    "healthCheckResponseReceived",
    "",
    "exitoso",
    "respuestaRecibida",
    "respuesta",
    "onConnected",
    "onReadyRead",
    "onErrorOccurred",
    "QLocalSocket::LocalSocketError",
    "error",
    "sendHealthCheck",
    "enviarSolicitud",
    "op",
    "payload"
};
#undef QT_MOC_LITERAL
} // unnamed namespace

Q_CONSTINIT static const uint qt_meta_data_InternalClient[] = {

 // content:
      10,       // revision
       0,       // classname
       0,    0, // classinfo
       8,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       2,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    1,   62,    2, 0x06,    1 /* Public */,
       4,    1,   65,    2, 0x06,    3 /* Public */,

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
       6,    0,   68,    2, 0x08,    5 /* Private */,
       7,    0,   69,    2, 0x08,    6 /* Private */,
       8,    1,   70,    2, 0x08,    7 /* Private */,

 // methods: name, argc, parameters, tag, flags, initial metatype offsets
      11,    0,   73,    2, 0x02,    9 /* Public */,
      12,    2,   74,    2, 0x02,   10 /* Public */,
      12,    1,   79,    2, 0x22,   13 /* Public | MethodCloned */,

 // signals: parameters
    QMetaType::Void, QMetaType::Bool,    3,
    QMetaType::Void, QMetaType::QJsonObject,    5,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 9,   10,

 // methods: parameters
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString, QMetaType::QJsonObject,   13,   14,
    QMetaType::Void, QMetaType::QString,   13,

       0        // eod
};

Q_CONSTINIT const QMetaObject InternalClient::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_meta_stringdata_InternalClient.offsetsAndSizes,
    qt_meta_data_InternalClient,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_stringdata_InternalClient_t,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<InternalClient, std::true_type>,
        // method 'healthCheckResponseReceived'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        // method 'respuestaRecibida'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QJsonObject &, std::false_type>,
        // method 'onConnected'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onReadyRead'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onErrorOccurred'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<QLocalSocket::LocalSocketError, std::false_type>,
        // method 'sendHealthCheck'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'enviarSolicitud'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QJsonObject &, std::false_type>,
        // method 'enviarSolicitud'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>
    >,
    nullptr
} };

void InternalClient::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<InternalClient *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->healthCheckResponseReceived((*reinterpret_cast< std::add_pointer_t<bool>>(_a[1]))); break;
        case 1: _t->respuestaRecibida((*reinterpret_cast< std::add_pointer_t<QJsonObject>>(_a[1]))); break;
        case 2: _t->onConnected(); break;
        case 3: _t->onReadyRead(); break;
        case 4: _t->onErrorOccurred((*reinterpret_cast< std::add_pointer_t<QLocalSocket::LocalSocketError>>(_a[1]))); break;
        case 5: _t->sendHealthCheck(); break;
        case 6: _t->enviarSolicitud((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QJsonObject>>(_a[2]))); break;
        case 7: _t->enviarSolicitud((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (InternalClient::*)(bool );
            if (_t _q_method = &InternalClient::healthCheckResponseReceived; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (InternalClient::*)(const QJsonObject & );
            if (_t _q_method = &InternalClient::respuestaRecibida; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 1;
                return;
            }
        }
    }
}

const QMetaObject *InternalClient::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *InternalClient::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_InternalClient.stringdata0))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int InternalClient::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
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
void InternalClient::healthCheckResponseReceived(bool _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void InternalClient::respuestaRecibida(const QJsonObject & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
