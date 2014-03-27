#pragma once
#include <exception>
#include <QString>
#include <QFile>
#include <logger/QsLog.h>

class MMCError : public std::exception
{
public:
	MMCError(QString cause)
	{
		exceptionCause = cause;
		QLOG_ERROR() << "Exception: " + cause;
	};
	virtual ~MMCError() noexcept
	{
	}
	virtual const char *what() const noexcept
	{
		return exceptionCause.toLocal8Bit();
	};
	virtual QString cause() const
	{
		return exceptionCause;
	}

private:
	QString exceptionCause;
};

class FileOpenError : public MMCError
{
public:
	FileOpenError(const QFile &file)
		: MMCError(QObject::tr("Unable to open %1: %2").arg(file.fileName(), file.errorString()))
	{
	}
	FileOpenError(const QFile *file)
		: FileOpenError(*file)
	{
	}
};
