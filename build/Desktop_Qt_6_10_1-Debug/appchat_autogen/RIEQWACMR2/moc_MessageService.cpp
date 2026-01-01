/****************************************************************************
** Meta object code from reading C++ file 'MessageService.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.10.1)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../src/service/MessageService.h"
#include <QtCore/qmetatype.h>
#include <QtCore/QList>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'MessageService.h' doesn't include <QObject>."
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
struct qt_meta_tag_ZN14MessageServiceE_t {};
} // unnamed namespace

template <> constexpr inline auto MessageService::qt_create_metaobjectdata<qt_meta_tag_ZN14MessageServiceE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "MessageService",
        "loginResult",
        "",
        "success",
        "msg",
        "registerResult",
        "sendMessageResponse",
        "ok",
        "message",
        "usersReceived",
        "QList<QJsonObject>",
        "users",
        "historyReceived",
        "peer",
        "messages",
        "incomingMessage",
        "QJsonObject",
        "commandError",
        "command",
        "onResponse",
        "obj"
    };

    QtMocHelpers::UintData qt_methods {
        // Signal 'loginResult'
        QtMocHelpers::SignalData<void(bool, QString)>(1, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Bool, 3 }, { QMetaType::QString, 4 },
        }}),
        // Signal 'registerResult'
        QtMocHelpers::SignalData<void(bool, QString)>(5, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Bool, 3 }, { QMetaType::QString, 4 },
        }}),
        // Signal 'sendMessageResponse'
        QtMocHelpers::SignalData<void(bool, QString)>(6, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Bool, 7 }, { QMetaType::QString, 8 },
        }}),
        // Signal 'usersReceived'
        QtMocHelpers::SignalData<void(QVector<QJsonObject>)>(9, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 10, 11 },
        }}),
        // Signal 'historyReceived'
        QtMocHelpers::SignalData<void(QString, QVector<QJsonObject>)>(12, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 13 }, { 0x80000000 | 10, 14 },
        }}),
        // Signal 'incomingMessage'
        QtMocHelpers::SignalData<void(QJsonObject)>(15, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 16, 8 },
        }}),
        // Signal 'commandError'
        QtMocHelpers::SignalData<void(QString, QString)>(17, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 18 }, { QMetaType::QString, 8 },
        }}),
        // Slot 'onResponse'
        QtMocHelpers::SlotData<void(QJsonObject)>(19, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { 0x80000000 | 16, 20 },
        }}),
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<MessageService, qt_meta_tag_ZN14MessageServiceE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject MessageService::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN14MessageServiceE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN14MessageServiceE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN14MessageServiceE_t>.metaTypes,
    nullptr
} };

void MessageService::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<MessageService *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->loginResult((*reinterpret_cast<std::add_pointer_t<bool>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QString>>(_a[2]))); break;
        case 1: _t->registerResult((*reinterpret_cast<std::add_pointer_t<bool>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QString>>(_a[2]))); break;
        case 2: _t->sendMessageResponse((*reinterpret_cast<std::add_pointer_t<bool>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QString>>(_a[2]))); break;
        case 3: _t->usersReceived((*reinterpret_cast<std::add_pointer_t<QList<QJsonObject>>>(_a[1]))); break;
        case 4: _t->historyReceived((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QList<QJsonObject>>>(_a[2]))); break;
        case 5: _t->incomingMessage((*reinterpret_cast<std::add_pointer_t<QJsonObject>>(_a[1]))); break;
        case 6: _t->commandError((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QString>>(_a[2]))); break;
        case 7: _t->onResponse((*reinterpret_cast<std::add_pointer_t<QJsonObject>>(_a[1]))); break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        if (QtMocHelpers::indexOfMethod<void (MessageService::*)(bool , QString )>(_a, &MessageService::loginResult, 0))
            return;
        if (QtMocHelpers::indexOfMethod<void (MessageService::*)(bool , QString )>(_a, &MessageService::registerResult, 1))
            return;
        if (QtMocHelpers::indexOfMethod<void (MessageService::*)(bool , QString )>(_a, &MessageService::sendMessageResponse, 2))
            return;
        if (QtMocHelpers::indexOfMethod<void (MessageService::*)(QVector<QJsonObject> )>(_a, &MessageService::usersReceived, 3))
            return;
        if (QtMocHelpers::indexOfMethod<void (MessageService::*)(QString , QVector<QJsonObject> )>(_a, &MessageService::historyReceived, 4))
            return;
        if (QtMocHelpers::indexOfMethod<void (MessageService::*)(QJsonObject )>(_a, &MessageService::incomingMessage, 5))
            return;
        if (QtMocHelpers::indexOfMethod<void (MessageService::*)(QString , QString )>(_a, &MessageService::commandError, 6))
            return;
    }
}

const QMetaObject *MessageService::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *MessageService::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN14MessageServiceE_t>.strings))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int MessageService::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 8)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 8;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 8)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 8;
    }
    return _id;
}

// SIGNAL 0
void MessageService::loginResult(bool _t1, QString _t2)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 0, nullptr, _t1, _t2);
}

// SIGNAL 1
void MessageService::registerResult(bool _t1, QString _t2)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 1, nullptr, _t1, _t2);
}

// SIGNAL 2
void MessageService::sendMessageResponse(bool _t1, QString _t2)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 2, nullptr, _t1, _t2);
}

// SIGNAL 3
void MessageService::usersReceived(QVector<QJsonObject> _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 3, nullptr, _t1);
}

// SIGNAL 4
void MessageService::historyReceived(QString _t1, QVector<QJsonObject> _t2)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 4, nullptr, _t1, _t2);
}

// SIGNAL 5
void MessageService::incomingMessage(QJsonObject _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 5, nullptr, _t1);
}

// SIGNAL 6
void MessageService::commandError(QString _t1, QString _t2)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 6, nullptr, _t1, _t2);
}
QT_WARNING_POP
