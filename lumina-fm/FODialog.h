//===========================================
//  Lumina-DE source code
//  Copyright (c) 2014, Ken Moore
//  Available under the 3-clause BSD license
//  See the LICENSE file for full details
//===========================================
//  This is the dialog for showing currently running file operations
//===========================================
#ifndef _LUMINA_FILE_MANAGER_FILE_OP_DIALOG_H
#define _LUMINA_FILE_MANAGER_FILE_OP_DIALOG_H

// Qt includes
#include <QDialog>
#include <QMessageBox>
#include <QStringList>
#include <QTimer>
#include <QFileInfo>
#include <QDir>
#include <QFile>
#include <QThread>

// libLumina includes
#include <LuminaXDG.h>
#include <LuminaUtils.h>

namespace Ui{
	class FODialog;
};

class FOWorker : public QObject{
	Q_OBJECT
public:
	//variables that need to be set before starting the operations
	QStringList ofiles, nfiles; //original/new files
	bool isRM, isCP, isRESTORE, isMV;
	bool stopped;
	int overwrite; // [-1= auto, 0= no overwrite, 1= overwrite]


	FOWorker() : QObject(){
	  isRM = isCP = isRESTORE = isMV = stopped = false;
	  overwrite = -1; //auto
	}
	~FOWorker(){}
		
public slots:
	void slotStartOperations();

private:
	QStringList subfiles(QString dirpath, bool dirsfirst = false, long long device_id=-1); //recursive function for fetching all "child" files/dirs (dirs last by default)
	QString newFileName(QString path);
	QStringList removeItem(QString path, bool recursive = false, long long device_id=-1);
	QStringList copyItem(QString oldpath, QString newpath);
  QString getFullPath(const QString &file) const;
  long long getDeviceId(const QString &fullpath) const;
  bool isMountPoint(const QString &fullpath) const;

signals:
	void startingItem(int, int, QString, QString); //current number, total number, Old File, New File (if appropriate)
	void finished(QStringList); //errors returned
};

class FODialog : public QDialog{
	Q_OBJECT
public:
	FODialog(QWidget *parent = 0);
	~FODialog();

	bool noerrors;

	void setOverwrite(bool);
	bool RemoveFiles(QStringList paths);
	bool CopyFiles(QStringList oldPaths, QStringList newPaths); 	  //same permissions as old files
	bool RestoreFiles(QStringList oldPaths, QStringList newPaths); //user/group rw permissions
	bool MoveFiles(QStringList oldPaths, QStringList newPaths);     //no change in permissions

private:
	Ui::FODialog *ui;
	QThread *WorkThread;
	FOWorker *Worker;

	bool CheckOverwrite(); //Returns "true" if it is ok to start the procedure

private slots:
	void on_push_stop_clicked();
	void UpdateItem(int, int, QString, QString);
	void WorkDone(QStringList);
};

#endif
