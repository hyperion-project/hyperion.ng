#ifndef WEAKCONNECT_H
#define WEAKCONNECT_H

#include <type_traits>

// Qt includes
#include <QObject>

template <typename Func1, typename Func2, typename std::enable_if_t<std::is_member_pointer<Func2>::value, int> = 0>
static inline QMetaObject::Connection weakConnect(typename QtPrivate::FunctionPointer<Func1>::Object* sender,
												  Func1 signal,
												  typename QtPrivate::FunctionPointer<Func2>::Object* receiver,
												  Func2 slot)
{
	QMetaObject::Connection conn_normal = QObject::connect(sender, signal, receiver, slot);

	QMetaObject::Connection* conn_delete = new QMetaObject::Connection();

	*conn_delete = QObject::connect(sender, signal, [conn_normal, conn_delete]() {
		QObject::disconnect(conn_normal);
		QObject::disconnect(*conn_delete);
		delete conn_delete;
	});
	return conn_normal;
}

template <typename Func1, typename Func2, typename std::enable_if_t<!std::is_member_pointer<Func2>::value, int> = 0>
static inline QMetaObject::Connection weakConnect(typename QtPrivate::FunctionPointer<Func1>::Object* sender,
												  Func1 signal,
												  Func2 slot)
{
	QMetaObject::Connection conn_normal = QObject::connect(sender, signal, slot);

	QMetaObject::Connection* conn_delete = new QMetaObject::Connection();

	*conn_delete = QObject::connect(sender, signal, [conn_normal, conn_delete]() {
		QObject::disconnect(conn_normal);
		QObject::disconnect(*conn_delete);
		delete conn_delete;
	});
	return conn_normal;
}

template <typename Func1, typename Func2, typename std::enable_if_t<!std::is_member_pointer<Func2>::value, int> = 0>
static inline QMetaObject::Connection weakConnect(typename QtPrivate::FunctionPointer<Func1>::Object* sender,
												  Func1 signal,
												  typename QtPrivate::FunctionPointer<Func2>::Object* receiver,
												  Func2 slot)
{
	Q_UNUSED(receiver);
	QMetaObject::Connection conn_normal = QObject::connect(sender, signal, slot);

	QMetaObject::Connection* conn_delete = new QMetaObject::Connection();

	*conn_delete = QObject::connect(sender, signal, [conn_normal, conn_delete]() {
		QObject::disconnect(conn_normal);
		QObject::disconnect(*conn_delete);
		delete conn_delete;
	});
	return conn_normal;
}

#endif // WEAKCONNECT_H
