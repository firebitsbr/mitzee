/**
# Copyright (C) 2012-2014 Chincisan Octavian-Marius(mariuschincisan@gmail.com) - getic.net - N/A
#           FOR HOME USE ONLY. For corporate  please contact me
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
*/

#include "dialog.h"
#include "ui_dialog.h"
#include "udpsocks.cpp"
#include <QMessageBox>
#include <QListView>
#include <QAbstractItemModel>
#include <QStandardItemModel>

Dialog::Dialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Dialog)
{
    ui->setupUi(this);
    _listModel = new QStandardItemModel();
     ui->listView->setModel(_listModel);
     ui->pushButton->setEnabled(0);

     ui->progressBar->setMaximum(1024);
     ui->progressBar->setMinimum(0);
     ui->progressBar->setValue(0);

}

Dialog::~Dialog()
{
    delete ui;
}

void Dialog::Box(const char* t)
{
    QMessageBox msgBox;
    msgBox.setText(t);
    msgBox.exec();
}


void Dialog::on_pushButton_clicked()
{
    std::vector<Udper*>  udps;
    std::vector<std::string>  ifs;

    Udper::get_local_ifaces(ifs);
    if(ifs.empty())
    {
        Box("cannot find any ethernet interfaces");
        return;
    }
    ui->progressBar->setValue(0);
    QAbstractItemModel* pm = ui->listView->model();

    while(pm && pm->rowCount())
        pm->removeRows(0,1);

    QString qs = ui->lineEdit->text();
    ::strcpy(PI_QUERY, qs.toUtf8().constData());

    ui->progressBar->setMaximum(4*256*ifs.size());

    std::vector<std::string>::iterator i = ifs.begin();
    for(; i!=ifs.end(); ++i)
    {
        std::string is = *i;
        Udper* pu = new Udper(is);
        udps.push_back(pu);
    }


    std::set<Udper*>    done;
    std::vector<Udper*>::iterator u ;
    int progress=0;
    for(int k=0; k<4; ++k)
    {
        u = udps.begin();
        for(; u!=udps.end(); ++u)
        {
            Udper* pu = (*u);
            int rk = pu->discover(progress,this);
            if(rk>0 && done.find(pu) == done.end())
            {
                done.insert(pu);
            }
        }
    }

    int k=1;
    std::set<Udper*>::iterator si = done.begin();
    for(;si!=done.end();si++)
    {
        const std::set<std::string>& infos = (*si)->rem_ips();
        std::set<std::string>::const_iterator info=infos.begin();

        for(;info!=infos.end();info++)
        {
            QStandardItem* itm = new QStandardItem(info->c_str());
            _listModel->appendRow(itm);
            ++k;
        }
    }
    u = udps.begin();
    for(; u!=udps.end(); ++u)
    {
        delete *u;
    }
    ui->progressBar->setFormat("Scan complete.");

}

void Dialog::on_lineEdit_textEdited(const QString &arg1)
{

    ui->pushButton->setEnabled(arg1.length()>0);

}
