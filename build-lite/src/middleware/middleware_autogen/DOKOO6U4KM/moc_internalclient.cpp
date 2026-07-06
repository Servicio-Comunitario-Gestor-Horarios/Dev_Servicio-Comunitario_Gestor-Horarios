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
    uint offsetsAndSizes[20];
    char stringdata0[15];
    char stringdata1[28];
    char stringdata2[1];
    char stringdata3[8];
    char stringdata4[12];
    char stringdata5[12];
    char stringdata6[16];
    char stringdata7[31];
    char stringdata8[6];
    char stringdata9[16];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(sizeof(qt_meta_stringdata_InternalClient_t::offsetsAndSizes) + ofs), len 
Q_CONSTINIT static const qt_meta_stringdata_InternalClient_t qt_meta_stringdata_InternalClient = {
    {
        QT_MOC_LITERAL(0, 14),  // "InternalClient"
        QT_MOC_LITERAL(15, 27),  // "healthCheckResponseReceived"
        QT_MOC_LITERAL(43, 0),  // ""
        QT_MOC_LITERAL(44, 7),  // "success"
        QT_MOC_LITERAL(52, 11),  // "onConnected"
        QT_MOC_LITERAL(64, 11),  // "onReadyRead"
        QT_MOC_LITERAL(76, 15),  // "onErrorOccurred"
        QT_MOC_LITERAL(92, 30),  // "QLocalSocket::LocalSocketError"
        QT_MOC_LITERAL(123, 5),  // "error"
        QT_MOC_LITERAL(129, 15)   // "sendHealthCheck"
    },
    "InternalClient",
    "healthCheckResponseReceived",
    "",
    "success",
    "onConnected",
    "onReadyRead",
    "onErrorOccurred",
    "QLocalSocket::LocalSocketError",
    "error",
    "sendHealthCheck"
};
#undef QT_MOC_LITERAL
} // unnamed namespace

Q_CONSTINIT static const uint qt_meta_data_InternalClient[] = {

 // content:
      10,       // revision
       0,       // classname
       0,    0, // classinfo
       5,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       1,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    1,   44,    2, 0x06,    1 /* Public */,

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
       4,    0,   47,    2, 0x08,    3 /* Private */,
       5,    0,   48,    2, 0x08,    4 /* Private */,
       6,    1,   49,    2, 0x08,    5 /* Private */,

 // methods: name, argc, parameters, tag, flags, initial metatype offsets
       9,    0,   52,    2, 0x02,    7 /* Public */,

 // signals: parameters
    QMetaType::Void, QMetaType::Bool,    3,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 7,    8,

 // methods: parameters
    QMetaType::Void,

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
        // method 'onConnected'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onReadyRead'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onErrorOccurred'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<QLocalSocket::LocalSocketError, std::false_type>,
        // method 'sendHealthCheck'
        QtPrivate::TypeAndForceComplete<void, std::false_type>
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
        case 1: _t->onConnected(); break;
        case 2: _t->onReadyRead(); break;
        case 3: _t->onErrorOccurred((*reinterpret_cast< std::add_pointer_t<QLocalSocket::LocalSocketError>>(_a[1]))); break;
        case 4: _t->sendHealthCheck(); break;
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
        if (_id < 5)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 5;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 5)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 5;
    }
    return _id;
}

// SIGNAL 0
void InternalClient::healthCheckResponseReceived(bool _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
