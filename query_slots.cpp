#include "ui_mainwindow.h"
#include "mainwindow.h"
#include "customitems.h"
#include "worker.h"
#include "query.h"
#include "serverinfo.h"
#include "settings.h"

extern QMap<int, QString> appIDMap;
extern QList<ServerInfo *> serverList;
extern QColor errorColor;
extern QColor queryingColor;

#define UPDATE_TIME 15

//TIMER TRIGGERED UPDATING
void MainWindow::TimedUpdate()
{
    static int run = 1;

    if(run % UPDATE_TIME == 0)
    {
        for(int i = 0; i < this->ui->browserTable->rowCount(); i++)
        {
            QTableWidgetItem *id = this->ui->browserTable->item(i, 0);
            int index = id->text().toInt();

            InfoQuery *infoQuery = new InfoQuery(this);

            infoQuery->query(&serverList.at(index-1)->host, serverList.at(index-1)->port, id);
        }
        if(run % 60 == 0)
        {
            this->UpdateSelectedItemInfo(false, true);
            run = 1;
        }
        else
        {
            this->UpdateSelectedItemInfo(false, false);
            run++;
        }
    }
    else
    {
        for(int i = 0; i < this->ui->playerTable->rowCount(); i++)
        {
            PlayerTimeTableItem *item = (PlayerTimeTableItem *)this->ui->playerTable->item(i, 3);

            item->updateTime(item->getTime()+1.0);
        }
        run++;
    }
}

void MainWindow::UpdateSelectedItemInfo(bool removeFirst, bool updateRules)
{
    if(this->ui->browserTable->selectedItems().size() == 0)
        return;

    QTableWidgetItem *item = this->ui->browserTable->selectedItems().at(0);
    int index = item->text().toInt();

    if(removeFirst)
    {
        while(this->ui->playerTable->rowCount() > 0)
        {
            this->ui->playerTable->removeRow(0);
        }
        if(updateRules)
        {
            while(this->ui->rulesTable->rowCount() > 0)
            {
                this->ui->rulesTable->removeRow(0);
            }
        }
    }

    pPlayerQuery = new PlayerQuery(this);
    pPlayerQuery->query(&serverList.at(index-1)->host, serverList.at(index-1)->port, item);

    if(updateRules)
    {
        pRulesQuery = new RulesQuery(this);
        pRulesQuery->query(&serverList.at(index-1)->host, serverList.at(index-1)->port, item);
    }
}

//QUERY INFO READY
void MainWindow::ServerInfoReady(InfoReply *reply, QTableWidgetItem *indexCell)
{
    int index = -1;
    int row = -1;

    for(int i = 0; i < this->ui->browserTable->rowCount(); i++)
    {
        QTableWidgetItem *cell = this->ui->browserTable->item(i, 0);

        if(cell == indexCell)
        {
            row = i;
            index = cell->text().toInt();
            break;
        }
    }

    if(index == -1)
    {
        if(reply)
            delete reply;
        return;
    }

    ServerInfo *info = serverList.at(index-1);

    if(!info || !info->isValid)
    {
        if(reply)
            delete reply;
        return;
    }

    if(reply && reply->success)
    {
        info->vac = reply->vac;
        info->appId = reply->appId;
        info->os = reply->os;
        info->tags = reply->tags;
        info->protocol = reply->protocol;
        info->version = reply->version;
        info->haveInfo = true;

        QTableWidgetItem *mod = new QTableWidgetItem();

        QImage icon;

        icon.load(appIDMap.value(info->appId, QString(":/icons/icons/hl2.gif")));

        mod->setData(Qt::DecorationRole, icon);

        this->ui->browserTable->setSortingEnabled(false);

        this->ui->browserTable->setItem(row, 1, mod);

        if(reply->vac)
        {
            QTableWidgetItem *vacItem = new QTableWidgetItem();
            vacItem->setData(Qt::DecorationRole, this->GetVACImage());
            this->ui->browserTable->setItem(row, 2, vacItem);
        }

        if(reply->visibility)
        {
            QTableWidgetItem *lockedItem = new QTableWidgetItem();
            lockedItem->setData(Qt::DecorationRole, this->GetLockImage());
            this->ui->browserTable->setItem(row, 3, lockedItem);
        }

        this->ui->browserTable->setItem(row, 4, new QTableWidgetItem(reply->hostname));
        QTableWidgetItem *mapItem = new QTableWidgetItem(reply->map);
        mapItem->setTextColor(errorColor);
        this->ui->browserTable->setItem(row, 5, mapItem);

        PlayerTableItem *playerItem = new PlayerTableItem();
        playerItem->players = reply->players;

        if(reply->players == reply->maxplayers)
            playerItem->setTextColor(errorColor);
        else
            playerItem->setTextColor(queryingColor);

        playerItem->setText(QString("%1/%2 (%3)").arg(QString::number(reply->players), QString::number(reply->maxplayers), QString::number(reply->bots)));

        this->ui->browserTable->setItem(row, 6, playerItem);

        this->ui->browserTable->setSortingEnabled(true);
        delete reply;
    }
    else
    {
        QTableWidgetItem *item = new QTableWidgetItem();

        item->setTextColor(errorColor);
        item->setText(QString("Failed to query %1, retrying in %2 seconds").arg(info->ipPort, QString::number(UPDATE_TIME)));
        this->ui->browserTable->setItem(row, 4, item);

        if(reply)
            delete reply;
    }
}

void MainWindow::PlayerInfoReady(QList<PlayerInfo> *list, QTableWidgetItem *indexCell)
{
    if(this->ui->browserTable->selectedItems().empty() || this->ui->browserTable->selectedItems().at(0) != indexCell)
    {
        if(list)
        {
            delete list;
        }

        return;
    }

    while(this->ui->playerTable->rowCount() > 0)
    {
        this->ui->playerTable->removeRow(0);
    }

    this->ui->playerTable->setSortingEnabled(false);

    for(int i = 0; i < list->size(); i++)
    {
        QTableWidgetItem *itemScore = new QTableWidgetItem();
        QTableWidgetItem *id = new QTableWidgetItem();

        id->setData(Qt::DisplayRole, i+1);
        itemScore->setData(Qt::DisplayRole, list->at(i).score);

        this->ui->playerTable->insertRow(i);
        this->ui->playerTable->setItem(i, 0, id);
        this->ui->playerTable->setItem(i, 1, new QTableWidgetItem(list->at(i).name));
        this->ui->playerTable->setItem(i, 2, itemScore);

        PlayerTimeTableItem *time = new PlayerTimeTableItem();
        time->updateTime(list->at(i).time);

        this->ui->playerTable->setItem(i, 3, time);
    }

    if(list)
    {
        delete list;
    }

    this->ui->playerTable->setSortingEnabled(true);
}

void MainWindow::RulesInfoReady(QList<RulesInfo> *list, QTableWidgetItem *indexCell)
{
    if(this->ui->browserTable->selectedItems().empty() || this->ui->browserTable->selectedItems().at(0) != indexCell)
    {
        if(list)
        {
            delete list;
        }

        return;
    }
    int index = this->ui->browserTable->selectedItems().at(0)->text().toInt();

    ServerInfo *info = serverList.at(index-1);
    if(info->haveInfo)
    {
        list->append(RulesInfo("vac", QString::number(info->vac)));
        list->append(RulesInfo("version", info->version));
        list->append(RulesInfo("appID", QString::number(info->appId)));
        list->append(RulesInfo("os", info->os));
    }

    while(this->ui->rulesTable->rowCount() > 0)
    {
        this->ui->rulesTable->removeRow(0);
    }

    this->ui->rulesTable->setSortingEnabled(false);

    for(int i = 0; i < list->size(); i++)
    {
        this->ui->rulesTable->insertRow(i);
        this->ui->rulesTable->setItem(i, 0, new QTableWidgetItem(list->at(i).name));
        this->ui->rulesTable->setItem(i, 1, new QTableWidgetItem(list->at(i).value));
    }

    if(list)
    {
        delete list;
    }

    this->ui->rulesTable->setSortingEnabled(true);
}
