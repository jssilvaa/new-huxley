/****************************************************************************
** Meta object code from reading C++ file 'ClientController.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.10.1)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../src/controller/ClientController.h"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'ClientController.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 69
#error "This file was generated using the moc from 6.10.1. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

#ifndef Q_CONSTINIT
#define Q_CONSTINIT
#endif

QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
QT_WARNING_DISABLE_GCC("-Wuseless-cast")
namespace {
struct qt_meta_tag_ZN16ClientControllerE_t {};
} // unnamed namespace

template <> constexpr inline auto ClientController::qt_create_metaobjectdata<qt_meta_tag_ZN16ClientControllerE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "ClientController",
        "connectedChanged",
        "",
        "authenticatedChanged",
        "currentPeerChanged",
        "currentPeerOnlineChanged",
        "clearChat",
        "showChat",
        "toast",
        "msg",
        "error",
        "onConnected",
        "onDisconnected",
        "onError",
        "start",
        "login",
        "user",
        "pass",
        "refreshUsers",
        "selectPeer",
        "peer",
        "sendMessage",
        "content",
        "unreadCount",
        "hasUnread",
        "connected",
        "authenticated",
        "currentPeer",
        "hasPeer",
        "messageService",
        "contacts",
        "chat",
        "currentPeerOnline"
    };

    QtMocHelpers::UintData qt_methods {
        // Signal 'connectedChanged'
        QtMocHelpers::SignalData<void()>(1, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'authenticatedChanged'
        QtMocHelpers::SignalData<void()>(3, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'currentPeerChanged'
        QtMocHelpers::SignalData<void()>(4, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'currentPeerOnlineChanged'
        QtMocHelpers::SignalData<void()>(5, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'clearChat'
        QtMocHelpers::SignalData<void()>(6, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'showChat'
        QtMocHelpers::SignalData<void()>(7, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'toast'
        QtMocHelpers::SignalData<void(QString)>(8, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 9 },
        }}),
        // Signal 'error'
        QtMocHelpers::SignalData<void(QString)>(10, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 9 },
        }}),
        // Slot 'onConnected'
        QtMocHelpers::SlotData<void()>(11, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onDisconnected'
        QtMocHelpers::SlotData<void()>(12, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onError'
        QtMocHelpers::SlotData<void(QString)>(13, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::QString, 9 },
        }}),
        // Method 'start'
        QtMocHelpers::MethodData<void()>(14, 2, QMC::AccessPublic, QMetaType::Void),
        // Method 'login'
        QtMocHelpers::MethodData<void(const QString &, const QString &)>(15, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 16 }, { QMetaType::QString, 17 },
        }}),
        // Method 'refreshUsers'
        QtMocHelpers::MethodData<void()>(18, 2, QMC::AccessPublic, QMetaType::Void),
        // Method 'selectPeer'
        QtMocHelpers::MethodData<void(const QString &)>(19, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 20 },
        }}),
        // Method 'sendMessage'
        QtMocHelpers::MethodData<void(const QString &)>(21, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 22 },
        }}),
        // Method 'unreadCount'
        QtMocHelpers::MethodData<int(const QString &) const>(23, 2, QMC::AccessPublic, QMetaType::Int, {{
            { QMetaType::QString, 16 },
        }}),
        // Method 'hasUnread'
        QtMocHelpers::MethodData<bool(const QString &) const>(24, 2, QMC::AccessPublic, QMetaType::Bool, {{
            { QMetaType::QString, 16 },
        }}),
    };
    QtMocHelpers::UintData qt_properties {
        // property 'connected'
        QtMocHelpers::PropertyData<bool>(25, QMetaType::Bool, QMC::DefaultPropertyFlags, 0),
        // property 'authenticated'
        QtMocHelpers::PropertyData<bool>(26, QMetaType::Bool, QMC::DefaultPropertyFlags, 1),
        // property 'currentPeer'
        QtMocHelpers::PropertyData<QString>(27, QMetaType::QString, QMC::DefaultPropertyFlags, 2),
        // property 'hasPeer'
        QtMocHelpers::PropertyData<bool>(28, QMetaType::Bool, QMC::DefaultPropertyFlags, 2),
        // property 'messageService'
        QtMocHelpers::PropertyData<QObject*>(29, QMetaType::QObjectStar, QMC::DefaultPropertyFlags | QMC::Constant),
        // property 'contacts'
        QtMocHelpers::PropertyData<QObject*>(30, QMetaType::QObjectStar, QMC::DefaultPropertyFlags | QMC::Constant),
        // property 'chat'
        QtMocHelpers::PropertyData<QObject*>(31, QMetaType::QObjectStar, QMC::DefaultPropertyFlags | QMC::Constant),
        // property 'currentPeerOnline'
        QtMocHelpers::PropertyData<bool>(32, QMetaType::Bool, QMC::DefaultPropertyFlags, 3),
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<ClientController, qt_meta_tag_ZN16ClientControllerE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject ClientController::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN16ClientControllerE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN16ClientControllerE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN16ClientControllerE_t>.metaTypes,
    nullptr
} };

void ClientController::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<ClientController *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->connectedChanged(); break;
        case 1: _t->authenticatedChanged(); break;
        case 2: _t->currentPeerChanged(); break;
        case 3: _t->currentPeerOnlineChanged(); break;
        case 4: _t->clearChat(); break;
        case 5: _t->showChat(); break;
        case 6: _t->toast((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 7: _t->error((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 8: _t->onConnected(); break;
        case 9: _t->onDisconnected(); break;
        case 10: _t->onError((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 11: _t->start(); break;
        case 12: _t->login((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QString>>(_a[2]))); break;
        case 13: _t->refreshUsers(); break;
        case 14: _t->selectPeer((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 15: _t->sendMessage((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 16: { int _r = _t->unreadCount((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1])));
            if (_a[0]) *reinterpret_cast<int*>(_a[0]) = std::move(_r); }  break;
        case 17: { bool _r = _t->hasUnread((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1])));
            if (_a[0]) *reinterpret_cast<bool*>(_a[0]) = std::move(_r); }  break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        if (QtMocHelpers::indexOfMethod<void (ClientController::*)()>(_a, &ClientController::connectedChanged, 0))
            return;
        if (QtMocHelpers::indexOfMethod<void (ClientController::*)()>(_a, &ClientController::authenticatedChanged, 1))
            return;
        if (QtMocHelpers::indexOfMethod<void (ClientController::*)()>(_a, &ClientController::currentPeerChanged, 2))
            return;
        if (QtMocHelpers::indexOfMethod<void (ClientController::*)()>(_a, &ClientController::currentPeerOnlineChanged, 3))
            return;
        if (QtMocHelpers::indexOfMethod<void (ClientController::*)()>(_a, &ClientController::clearChat, 4))
            return;
        if (QtMocHelpers::indexOfMethod<void (ClientController::*)()>(_a, &ClientController::showChat, 5))
            return;
        if (QtMocHelpers::indexOfMethod<void (ClientController::*)(QString )>(_a, &ClientController::toast, 6))
            return;
        if (QtMocHelpers::indexOfMethod<void (ClientController::*)(QString )>(_a, &ClientController::error, 7))
            return;
    }
    if (_c == QMetaObject::ReadProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 0: *reinterpret_cast<bool*>(_v) = _t->connected(); break;
        case 1: *reinterpret_cast<bool*>(_v) = _t->authenticated(); break;
        case 2: *reinterpret_cast<QString*>(_v) = _t->currentPeer(); break;
        case 3: *reinterpret_cast<bool*>(_v) = _t->hasPeer(); break;
        case 4: *reinterpret_cast<QObject**>(_v) = _t->messageService(); break;
        case 5: *reinterpret_cast<QObject**>(_v) = _t->contacts(); break;
        case 6: *reinterpret_cast<QObject**>(_v) = _t->chat(); break;
        case 7: *reinterpret_cast<bool*>(_v) = _t->currentPeerOnline(); break;
        default: break;
        }
    }
}

const QMetaObject *ClientController::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *ClientController::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN16ClientControllerE_t>.strings))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int ClientController::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 18)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 18;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 18)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 18;
    }
    if (_c == QMetaObject::ReadProperty || _c == QMetaObject::WriteProperty
            || _c == QMetaObject::ResetProperty || _c == QMetaObject::BindableProperty
            || _c == QMetaObject::RegisterPropertyMetaType) {
        qt_static_metacall(this, _c, _id, _a);
        _id -= 8;
    }
    return _id;
}

// SIGNAL 0
void ClientController::connectedChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void ClientController::authenticatedChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void ClientController::currentPeerChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 2, nullptr);
}

// SIGNAL 3
void ClientController::currentPeerOnlineChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 3, nullptr);
}

// SIGNAL 4
void ClientController::clearChat()
{
    QMetaObject::activate(this, &staticMetaObject, 4, nullptr);
}

// SIGNAL 5
void ClientController::showChat()
{
    QMetaObject::activate(this, &staticMetaObject, 5, nullptr);
}

// SIGNAL 6
void ClientController::toast(QString _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 6, nullptr, _t1);
}

// SIGNAL 7
void ClientController::error(QString _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 7, nullptr, _t1);
}
QT_WARNING_POP
