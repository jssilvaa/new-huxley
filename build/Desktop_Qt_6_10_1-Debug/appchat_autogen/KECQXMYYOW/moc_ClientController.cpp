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
        "registeringChanged",
        "currentPeerChanged",
        "currentPeerOnlineChanged",
        "focusContactsChanged",
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
        "registerUser",
        "showRegister",
        "showLogin",
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
        "currentPeerOnline",
        "focusContacts",
        "contactsProxy",
        "registering"
    };

    QtMocHelpers::UintData qt_methods {
        // Signal 'connectedChanged'
        QtMocHelpers::SignalData<void()>(1, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'authenticatedChanged'
        QtMocHelpers::SignalData<void()>(3, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'registeringChanged'
        QtMocHelpers::SignalData<void()>(4, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'currentPeerChanged'
        QtMocHelpers::SignalData<void()>(5, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'currentPeerOnlineChanged'
        QtMocHelpers::SignalData<void()>(6, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'focusContactsChanged'
        QtMocHelpers::SignalData<void()>(7, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'clearChat'
        QtMocHelpers::SignalData<void()>(8, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'showChat'
        QtMocHelpers::SignalData<void()>(9, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'toast'
        QtMocHelpers::SignalData<void(QString)>(10, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 11 },
        }}),
        // Signal 'error'
        QtMocHelpers::SignalData<void(QString)>(12, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 11 },
        }}),
        // Slot 'onConnected'
        QtMocHelpers::SlotData<void()>(13, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onDisconnected'
        QtMocHelpers::SlotData<void()>(14, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onError'
        QtMocHelpers::SlotData<void(QString)>(15, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::QString, 11 },
        }}),
        // Method 'start'
        QtMocHelpers::MethodData<void()>(16, 2, QMC::AccessPublic, QMetaType::Void),
        // Method 'login'
        QtMocHelpers::MethodData<void(const QString &, const QString &)>(17, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 18 }, { QMetaType::QString, 19 },
        }}),
        // Method 'registerUser'
        QtMocHelpers::MethodData<void(const QString &, const QString &)>(20, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 18 }, { QMetaType::QString, 19 },
        }}),
        // Method 'showRegister'
        QtMocHelpers::MethodData<void()>(21, 2, QMC::AccessPublic, QMetaType::Void),
        // Method 'showLogin'
        QtMocHelpers::MethodData<void()>(22, 2, QMC::AccessPublic, QMetaType::Void),
        // Method 'refreshUsers'
        QtMocHelpers::MethodData<void()>(23, 2, QMC::AccessPublic, QMetaType::Void),
        // Method 'selectPeer'
        QtMocHelpers::MethodData<void(const QString &)>(24, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 25 },
        }}),
        // Method 'sendMessage'
        QtMocHelpers::MethodData<void(const QString &)>(26, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 27 },
        }}),
        // Method 'unreadCount'
        QtMocHelpers::MethodData<int(const QString &) const>(28, 2, QMC::AccessPublic, QMetaType::Int, {{
            { QMetaType::QString, 18 },
        }}),
        // Method 'hasUnread'
        QtMocHelpers::MethodData<bool(const QString &) const>(29, 2, QMC::AccessPublic, QMetaType::Bool, {{
            { QMetaType::QString, 18 },
        }}),
    };
    QtMocHelpers::UintData qt_properties {
        // property 'connected'
        QtMocHelpers::PropertyData<bool>(30, QMetaType::Bool, QMC::DefaultPropertyFlags, 0),
        // property 'authenticated'
        QtMocHelpers::PropertyData<bool>(31, QMetaType::Bool, QMC::DefaultPropertyFlags, 1),
        // property 'currentPeer'
        QtMocHelpers::PropertyData<QString>(32, QMetaType::QString, QMC::DefaultPropertyFlags, 3),
        // property 'hasPeer'
        QtMocHelpers::PropertyData<bool>(33, QMetaType::Bool, QMC::DefaultPropertyFlags, 3),
        // property 'messageService'
        QtMocHelpers::PropertyData<QObject*>(34, QMetaType::QObjectStar, QMC::DefaultPropertyFlags | QMC::Constant),
        // property 'contacts'
        QtMocHelpers::PropertyData<QObject*>(35, QMetaType::QObjectStar, QMC::DefaultPropertyFlags | QMC::Constant),
        // property 'chat'
        QtMocHelpers::PropertyData<QObject*>(36, QMetaType::QObjectStar, QMC::DefaultPropertyFlags | QMC::Constant),
        // property 'currentPeerOnline'
        QtMocHelpers::PropertyData<bool>(37, QMetaType::Bool, QMC::DefaultPropertyFlags, 4),
        // property 'focusContacts'
        QtMocHelpers::PropertyData<bool>(38, QMetaType::Bool, QMC::DefaultPropertyFlags | QMC::Writable | QMC::StdCppSet, 5),
        // property 'contactsProxy'
        QtMocHelpers::PropertyData<QObject*>(39, QMetaType::QObjectStar, QMC::DefaultPropertyFlags | QMC::Constant),
        // property 'registering'
        QtMocHelpers::PropertyData<bool>(40, QMetaType::Bool, QMC::DefaultPropertyFlags, 2),
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
        case 2: _t->registeringChanged(); break;
        case 3: _t->currentPeerChanged(); break;
        case 4: _t->currentPeerOnlineChanged(); break;
        case 5: _t->focusContactsChanged(); break;
        case 6: _t->clearChat(); break;
        case 7: _t->showChat(); break;
        case 8: _t->toast((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 9: _t->error((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 10: _t->onConnected(); break;
        case 11: _t->onDisconnected(); break;
        case 12: _t->onError((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 13: _t->start(); break;
        case 14: _t->login((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QString>>(_a[2]))); break;
        case 15: _t->registerUser((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QString>>(_a[2]))); break;
        case 16: _t->showRegister(); break;
        case 17: _t->showLogin(); break;
        case 18: _t->refreshUsers(); break;
        case 19: _t->selectPeer((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 20: _t->sendMessage((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 21: { int _r = _t->unreadCount((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1])));
            if (_a[0]) *reinterpret_cast<int*>(_a[0]) = std::move(_r); }  break;
        case 22: { bool _r = _t->hasUnread((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1])));
            if (_a[0]) *reinterpret_cast<bool*>(_a[0]) = std::move(_r); }  break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        if (QtMocHelpers::indexOfMethod<void (ClientController::*)()>(_a, &ClientController::connectedChanged, 0))
            return;
        if (QtMocHelpers::indexOfMethod<void (ClientController::*)()>(_a, &ClientController::authenticatedChanged, 1))
            return;
        if (QtMocHelpers::indexOfMethod<void (ClientController::*)()>(_a, &ClientController::registeringChanged, 2))
            return;
        if (QtMocHelpers::indexOfMethod<void (ClientController::*)()>(_a, &ClientController::currentPeerChanged, 3))
            return;
        if (QtMocHelpers::indexOfMethod<void (ClientController::*)()>(_a, &ClientController::currentPeerOnlineChanged, 4))
            return;
        if (QtMocHelpers::indexOfMethod<void (ClientController::*)()>(_a, &ClientController::focusContactsChanged, 5))
            return;
        if (QtMocHelpers::indexOfMethod<void (ClientController::*)()>(_a, &ClientController::clearChat, 6))
            return;
        if (QtMocHelpers::indexOfMethod<void (ClientController::*)()>(_a, &ClientController::showChat, 7))
            return;
        if (QtMocHelpers::indexOfMethod<void (ClientController::*)(QString )>(_a, &ClientController::toast, 8))
            return;
        if (QtMocHelpers::indexOfMethod<void (ClientController::*)(QString )>(_a, &ClientController::error, 9))
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
        case 8: *reinterpret_cast<bool*>(_v) = _t->focusContacts(); break;
        case 9: *reinterpret_cast<QObject**>(_v) = _t->contactsProxy(); break;
        case 10: *reinterpret_cast<bool*>(_v) = _t->registering(); break;
        default: break;
        }
    }
    if (_c == QMetaObject::WriteProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 8: _t->setFocusContacts(*reinterpret_cast<bool*>(_v)); break;
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
        if (_id < 23)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 23;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 23)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 23;
    }
    if (_c == QMetaObject::ReadProperty || _c == QMetaObject::WriteProperty
            || _c == QMetaObject::ResetProperty || _c == QMetaObject::BindableProperty
            || _c == QMetaObject::RegisterPropertyMetaType) {
        qt_static_metacall(this, _c, _id, _a);
        _id -= 11;
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
void ClientController::registeringChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 2, nullptr);
}

// SIGNAL 3
void ClientController::currentPeerChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 3, nullptr);
}

// SIGNAL 4
void ClientController::currentPeerOnlineChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 4, nullptr);
}

// SIGNAL 5
void ClientController::focusContactsChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 5, nullptr);
}

// SIGNAL 6
void ClientController::clearChat()
{
    QMetaObject::activate(this, &staticMetaObject, 6, nullptr);
}

// SIGNAL 7
void ClientController::showChat()
{
    QMetaObject::activate(this, &staticMetaObject, 7, nullptr);
}

// SIGNAL 8
void ClientController::toast(QString _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 8, nullptr, _t1);
}

// SIGNAL 9
void ClientController::error(QString _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 9, nullptr, _t1);
}
QT_WARNING_POP
