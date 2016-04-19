//===========================================
//  Lumina-DE source code
//  Copyright (c) 2015, Ken Moore
//  Available under the 3-clause BSD license
//  See the LICENSE file for full details
//===========================================
#include <QtConcurrent/QtConcurrentRun>
#include "MainUI.h"
#include "ui_MainUI.h"

#include <QFileDialog>
#include <QMessageBox>

#include <LuminaUtils.h>
#include <LuminaOS.h>

LFileInfo INFO = LFileInfo("");
	
MainUI::MainUI() : QDialog(), ui(new Ui::MainUI){
  ui->setupUi(this); //load the designer form
  canwrite = false;
  UpdateIcons(); //Set all the icons in the dialog
  folder_size_refresh_interval = new QTimer(this);
  SetupConnections();
}

//=============
//      PUBLIC
//=============
void MainUI::LoadFile(QString path, QString type){
  INFO = LFileInfo(path);
  if(INFO.exists()){ canwrite = INFO.isWritable(); }
  else{
    //See if the containing directory can be written
    QFileInfo chk(INFO.absolutePath());
    canwrite = (chk.isDir() && chk.isWritable());
  }
  if(!INFO.exists() && !type.isEmpty()){
    //Set the proper type flag on the shortcut
    if(type=="APP"){ INFO.XDG()->type = XDGDesktop::APP; }
    else if(type=="LINK"){ INFO.XDG()->type = XDGDesktop::LINK; }
  }
	  
  //First load the general file information
  ui->label_file_name->setText( INFO.fileName() );
  ui->label_file_mimetype->setText( INFO.mimetype() );
  if(!INFO.isDir()){ ui->label_file_size->setText( LUtils::BytesToDisplaySize( INFO.size() ) ); }
  else {
    ui->label_file_size->setText(tr("---Calculating---"));
    quid = QUuid::createUuid();
    QtConcurrent::run(&LUtils::getDirSize, INFO.absoluteFilePath(), quid);
    get_folder_information();
    // Refresh information every second (1000 ms)
    folder_size_refresh_interval->start(1000);
  }
  ui->label_file_owner->setText(INFO.owner());
  ui->label_file_group->setText(INFO.group());
  ui->label_file_created->setText( INFO.created().toString(Qt::SystemLocaleLongDate) );
  ui->label_file_modified->setText( INFO.lastModified().toString(Qt::SystemLocaleLongDate) );
  //Get the file permissions
  QString perms;
  if(INFO.isReadable() && INFO.isWritable()){ perms = tr("Read/Write"); }
  else if(INFO.isReadable()){ perms = tr("Read Only"); }
  else if(INFO.isWritable()){ perms = tr("Write Only"); }
  else{ perms = tr("No Access"); }
  ui->label_file_perms->setText(perms);
  //Now the special "type" for the file
  QString ftype;
  if(INFO.suffix().toLower()=="desktop"){ ftype = tr("XDG Shortcut"); }
  else if(INFO.isDir()){ ftype = tr("Directory"); }
  else if(INFO.isExecutable()){ ftype = tr("Binary"); }
  else{ ftype = INFO.suffix().toUpper(); }
  if(INFO.isHidden()){ ftype = QString(tr("Hidden %1")).arg(type); }
  ui->label_file_type->setText(ftype);
  //Now load the icon for the file
  if(INFO.isImage()){
    ui->label_file_icon->setPixmap( QPixmap(INFO.absoluteFilePath()).scaledToHeight(64) );
  }else{
    ui->label_file_icon->setPixmap( LXDG::findIcon( INFO.iconfile(), "unknown").pixmap(QSize(64,64)) );
  }
  //Now load the special XDG desktop info
  if(INFO.isDesktopFile()){
  
    if(INFO.XDG()->type == XDGDesktop::APP){
      ui->line_xdg_command->setText(INFO.XDG()->exec);
      ui->line_xdg_wdir->setText(INFO.XDG()->path);
      ui->check_xdg_useTerminal->setChecked( INFO.XDG()->useTerminal );
      ui->check_xdg_startupNotify->setChecked( INFO.XDG()->startupNotify );
    }else if(INFO.XDG()->type==XDGDesktop::LINK){
      //Hide the options that are unavailable for links
      //Command  line (exec)
      ui->line_xdg_command->setVisible(false);
      ui->tool_xdg_getCommand->setVisible(false);
      ui->lblCommand->setVisible(false);
      //Options
      ui->lblOptions->setVisible(false);
      ui->check_xdg_useTerminal->setVisible(false);
      ui->check_xdg_startupNotify->setVisible(false);
      //Now load the variables for this type of shortcut
      ui->lblWorkingDir->setText(tr("URL:"));
      ui->line_xdg_wdir->setText( INFO.XDG()->url );
      ui->tool_xdg_getDir->setVisible(false); //the dir selection button
      
    }
    ui->line_xdg_name->setText(INFO.XDG()->name);	
    ui->line_xdg_comment->setText(INFO.XDG()->comment);
    ui->push_xdg_getIcon->setWhatsThis( INFO.XDG()->icon );
    ReloadAppIcon();
    ui->push_save->setVisible(true);
    ui->push_save->setEnabled(false);
  }else{
    xdgvaluechanged(); //just do the disables here
    //Also remove the xdg tab
    if(ui->tabWidget->count()>1){ ui->tabWidget->removeTab(1); }
  }
  //Setup the tab 
  if(type.isEmpty()){  ui->tabWidget->setCurrentIndex(0); }
  else if(ui->tabWidget->count()>1){ ui->tabWidget->setCurrentIndex(1); }
  //Hide the tab bar (the autoHideTabBar setting was not added until Qt 5.4)
  if(ui->tabWidget->count() < 2){ ui->tabWidget->tabBar()->hide(); }
}

void MainUI::UpdateIcons(){
  this->setWindowIcon(LXDG::findIcon("document-preview","unknown"));
  ui->push_close->setIcon( LXDG::findIcon("dialog-close","") );
  ui->push_save->setIcon( LXDG::findIcon("document-save","") );
  ui->tool_xdg_getCommand->setIcon( LXDG::findIcon("edit-find-project","") );
  ui->tool_xdg_getDir->setIcon( LXDG::findIcon("document-open","") );	
}

//==============
//     PRIVATE
//==============
void MainUI::ReloadAppIcon(){
  ui->push_xdg_getIcon->setIcon( LXDG::findIcon(ui->push_xdg_getIcon->whatsThis(),"") );
}

// Initialization procedures
void MainUI::SetupConnections(){
  connect(ui->line_xdg_command, SIGNAL(editingFinished()), this, SLOT(xdgvaluechanged()) );
  connect(ui->line_xdg_comment, SIGNAL(editingFinished()), this, SLOT(xdgvaluechanged()) );
  connect(ui->line_xdg_name, SIGNAL(editingFinished()), this, SLOT(xdgvaluechanged()) );
  connect(ui->line_xdg_wdir, SIGNAL(editingFinished()), this, SLOT(xdgvaluechanged()) );
  connect(ui->check_xdg_useTerminal, SIGNAL(clicked()), this, SLOT(xdgvaluechanged()) );
  connect(ui->check_xdg_startupNotify, SIGNAL(clicked()), this, SLOT(xdgvaluechanged()) );
  connect(folder_size_refresh_interval, SIGNAL(timeout()), this, SLOT(get_folder_information()));
}

//UI Buttons
void MainUI::on_push_close_clicked(){
  LUtils::abortGetDirSize();
  if(ui->push_save->isEnabled()){
    //Still have unsaved changes
    //TO-DO - prompt for whether to save the changes
	  
  }
  this->close();
}

void MainUI::on_push_save_clicked(){
  //Save all the xdg values into the structure
  if(!INFO.isDesktopFile() || !canwrite){ return; }
  XDGDesktop XDG = *INFO.XDG();;
  //Now change the structure
  XDG.name = ui->line_xdg_name->text();
  XDG.genericName = ui->line_xdg_name->text().toLower();
  XDG.comment = ui->line_xdg_comment->text();
  XDG.icon = ui->push_xdg_getIcon->whatsThis();
  //Now do the type-specific fields
  if(XDG.type==XDGDesktop::APP){
    XDG.exec = ui->line_xdg_command->text();
    XDG.tryexec = ui->line_xdg_command->text().section(" ",0,0); //use the first word/binary for the existance check
    XDG.path = ui->line_xdg_wdir->text(); //working dir/path
    XDG.useTerminal = ui->check_xdg_useTerminal->isChecked();
    XDG.startupNotify = ui->check_xdg_startupNotify->isChecked();
  }else if(XDG.type==XDGDesktop::LINK){
    XDG.url = ui->line_xdg_wdir->text(); //we re-used this field
  }
  //Clear any info which this utility does not support at the moment
  XDG.actionList.clear();
  XDG.actions.clear();
  //Now save the structure to file
  bool saved = LXDG::saveDesktopFile( XDG, true); //Try to merge the file/structure as necessary
  qDebug() << "File Saved:" << saved;
  ui->push_save->setEnabled( !saved );
  if(saved){
    //Re-load the file info
    LoadFile(INFO.absoluteFilePath());
  }
}

void MainUI::on_tool_xdg_getCommand_clicked(QString prev){
  //Find a binary to run
  QString dir = prev; //start with the previous attempt (if there was one)
  if(dir.isEmpty()){ ui->line_xdg_command->text(); }//then try current selection
  if(dir.isEmpty()){ dir = LOS::AppPrefix()+"bin"; } //then open the application binaries directory
  QString file = QFileDialog::getOpenFileName(this, tr("Select a binary"), dir );
  if(file.isEmpty()){ return; } //cancelled
  if(!LUtils::isValidBinary(file)){
    QMessageBox::warning(this, tr("Error"), tr("Invalid selection: Not a valid executable"));
    on_tool_xdg_getCommand_clicked(file);
    return;
  }
  ui->line_xdg_command->setText(file);
  xdgvaluechanged();
}

void MainUI::on_tool_xdg_getDir_clicked(){
  //Find a directory
  QString dir = ui->line_xdg_wdir->text();
  if(dir.isEmpty()){ dir = QDir::homePath(); }
  dir = QFileDialog::getExistingDirectory(this, tr("Select a directory"), dir);
  if(dir.isEmpty()){ return; } //cancelled
  ui->line_xdg_wdir->setText(dir);
  xdgvaluechanged();
}

void MainUI::on_push_xdg_getIcon_clicked(){
  //Find an image file
  QString dir = ui->push_xdg_getIcon->whatsThis(); //then try current selection
  if(dir.isEmpty()){ dir = QDir::homePath(); }
  //Get the known file extensions
  QStringList ext = LUtils::imageExtensions();
  for(int i=0; i<ext.length(); i++){ ext[i].prepend("*."); } //turn them into valid filters
  QString file = QFileDialog::getOpenFileName(this, tr("Select an icon"), dir ,QString(tr("Images (%1);; All Files (*)")).arg(ext.join(" ")) );
  if(file.isEmpty()){ return; } //cancelled
  ui->push_xdg_getIcon->setWhatsThis(file);	
  ReloadAppIcon();
  xdgvaluechanged();
}

//XDG Value Changed
void MainUI::xdgvaluechanged(){
  if(INFO.isDesktopFile()){
    ui->push_save->setVisible(true);
    //Compare the current UI values to the file values
    ui->push_save->setEnabled(canwrite); //assume changed at this point
    // TO-DO
	  
  }else{
    ui->push_save->setVisible(false);
    ui->push_save->setEnabled(false);
  }
}

void MainUI::get_folder_information() {
  // Get information about folder size from Liblumina
  refresh_folder_size(LUtils::getFolderSize(quid), LUtils::getFileNumber(quid), LUtils::getFolderNumber(quid), LUtils::getOperationStatus(quid));
}

void MainUI::refresh_folder_size(quint64 size, quint64 files, quint64 folders, bool finished) {
  if(finished) {
    ui->label_file_size->setText( LUtils::BytesToDisplaySize( size ) + " -- " + tr(" Folders: ") + QString::number(folders) + " / " + tr("Files: ") + QString::number(files) );
    LUtils::deleteUuid(quid);
    // Stop timer when the operatiof getDirSize is finished
    folder_size_refresh_interval->stop();
  }
  else
    ui->label_file_size->setText( LUtils::BytesToDisplaySize( size ) + " -- " + tr(" Folders: ") + QString::number(folders) + " / " + tr("Files: ") + QString::number(files) + tr("  Calculating..." ));
}
