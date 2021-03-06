/*
#
# Draco Desktop Environment <https://dracolinux.org>
# Copyright (c) 2019, Ole-André Rodlie <ole.andre.rodlie@gmail.com>
# All rights reserved.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>
#
# This file incorporates work covered by the following copyright and
# permission notice:
#
# Lumina Desktop Environment (https://lumina-desktop.org)
# Copyright (c) 2012-2017, Ken Moore (moorekou@gmail.com)
# All rights reserved
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#     * Redistributions of source code must retain the above copyright
#       notice, this list of conditions and the following disclaimer.
#     * Redistributions in binary form must reproduce the above copyright
#       notice, this list of conditions and the following disclaimer in the
#       documentation and/or other materials provided with the distribution.
#     * Neither the name of the organization nor the
#       names of its contributors may be used to endorse or promote products
#       derived from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY
# DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
# ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
*/

//===========================================
//  Lumina-DE source code
//  Copyright (c) 2015, Ken Moore
//  Available under the 3-clause BSD license
//===========================================

#ifndef DESKTOP_LDESKTOP_PLUGIN_SPACE_H
#define DESKTOP_LDESKTOP_PLUGIN_SPACE_H

#include <QListWidget>
#include <QDropEvent>
#include <QDrag> // includes all the QDrag*Event classes
#include <QUrl>
#include <QMimeData>
#include <QSettings>
#include <QDebug>
#include <QFile>
#include <QDir>
#include <QFileInfo>
#include <QProcess>

#include "desktop-plugins/LDPlugin.h"

#include "qtcopydialog.h"
#include "qtfilecopier.h"

#define MIMETYPE QString("x-special/lumina-desktop-plugin")

class LDesktopPluginSpace : public QWidget
{
    Q_OBJECT

signals:
    void PluginRemovedByUser(QString ID);
    void IncreaseIcons(); // increase default icon sizes
    void DecreaseIcons(); // decrease default icon sizes
    void HideDesktopMenu();

public:
    LDesktopPluginSpace();
    ~LDesktopPluginSpace();

    void LoadItems(QStringList plugs, QStringList files);
    //void setShowGrid(bool show); This is already implemented in QTableView (inherited)
    void SetIconSize(int size);
    void ArrangeTopToBottom(bool ttb); //if false, will arrange left->right
    void cleanup();

    void setBackground(QPixmap pix); //should already be sized appropriately for this widget
    void setDesktopArea(QRect area);

public slots:
    void UpdateGeom(int oldgrid = -1);

private:
    QSettings *plugsettings;
    QStringList plugins, deskitems;
    QList<LDPlugin*> ITEMS;
    QPixmap wallpaper;
    QRect desktopRect;
    bool TopToBottom;
    float GRIDSIZE;

    int RoundUp(double num)
    {
        int out = num; // This will truncate the number
        if (out < num) { out++; } // need to increase by 1
        return out;
    }

    void addDesktopItem(QString filepath); // This will convert it into a valid Plugin ID automatically
    void addDesktopPlugin(QString plugID);


    QRect findOpenSpot(int gridwidth = 1,
                       int gridheight = 1,
                       int startRow = 0,
                       int startCol = 0,
                       bool reversed = false,
                       QString plugID = "");
    QRect findOpenSpot(QRect grid, QString plugID, bool recursive = false);

    QPoint posToGrid(QPoint pos)
    {
        pos.setX(RoundUp((pos.x()-desktopRect.x())/GRIDSIZE));
        pos.setY(RoundUp((pos.y()-desktopRect.y())/GRIDSIZE));
        return pos;
    }

    QPoint gridToPos(QPoint grid)
    {
        grid.setX((grid.x()*GRIDSIZE)+desktopRect.x());
        grid.setY((grid.y()*GRIDSIZE)+desktopRect.y());
        return grid;
    }

    QRect geomToGrid(QRect geom, int grid = -1)
    {
        if (grid<0) {
            // use the current grid size
            return QRect(RoundUp((geom.x()-desktopRect.x())/GRIDSIZE),
                         RoundUp((geom.y()-desktopRect.y())/GRIDSIZE),
                         RoundUp(geom.width()/GRIDSIZE),
                         RoundUp(geom.height()/GRIDSIZE));
        } else {
            // use the input grid size
            return QRect(RoundUp((geom.x()-desktopRect.x())/((double) grid)),
                         RoundUp((geom.y()-desktopRect.y())/((double) grid)),
                         RoundUp(geom.width()/((double) grid)),
                         RoundUp(geom.height()/((double) grid)) );
        }
    }

    QRect gridToGeom(QRect grid)
    {
        // This function incorporates the bottom/right edge matchins procedures (for incomplete last grid)
        QRect geom((grid.x()*GRIDSIZE)+desktopRect.x(),
                   (grid.y()*GRIDSIZE)+desktopRect.y(),
                   grid.width()*GRIDSIZE, grid.height()*GRIDSIZE);
        // Now check the edge conditions (last right/bottom grid points might be smaller than GRIDSIZE)
        if (geom.right() > desktopRect.right() && (geom.right()-desktopRect.right())<GRIDSIZE ) {
            geom.setRight(desktopRect.right()); // match up with the edge
        }
        if (geom.bottom() > desktopRect.bottom() && (geom.bottom() -desktopRect.bottom())<GRIDSIZE ) {
            geom.setBottom(desktopRect.bottom()); // match up with the edge
        }
        return geom;
    }

    // Internal simplification for setting up a drag event
    void setupDrag(QString id, QString type)
    {
        QMimeData *mime = new QMimeData;
        mime->setData(MIMETYPE, QString(type+"::::"+id).toLocal8Bit());
        // If this is a desktop file - also add it to the generic URI list mimetype
        if (id.startsWith("applauncher::")) {
            QList<QUrl> urilist;
            urilist << QUrl::fromLocalFile( id.section("---",0,0).section("::",1,50) );
            mime->setUrls(urilist);
        }
        // Create the drag structure
        QDrag *drag = new QDrag(this);
        drag->setMimeData(mime);
        drag->exec(Qt::CopyAction);
    }

    bool ValidGrid(QRect grid)
    {
        // This just checks that the grid coordinates are not out of bounds - should still run ValidGeometry() below with the actual pixel geom
        if (grid.x()<0|| grid.y()<0 || grid.width()<0 || grid.height()<0) { return false; }
        else if ((grid.x()+grid.width()) > RoundUp(desktopRect.width()/GRIDSIZE)) { return false; }
        else if ((grid.y()+grid.height()) > RoundUp(desktopRect.height()/GRIDSIZE)) { return false; }
        // Final Check - don't let 1x1 items occupy the last row/column (not full size)
        else if (grid.width()==1 &&
                 grid.height()==1 &&
                 (grid.x()==RoundUp(desktopRect.width()/GRIDSIZE) ||
                  grid.y()==RoundUp(desktopRect.height()/GRIDSIZE))) { return false; }
        return true;
    }

    bool ValidGeometry(QString id, QRect geom) {
        // First check that it is within the desktop area completely
        // Note that "this->geometry()" is not in the same coordinate space as the geometry inputs
        if (!desktopRect.contains(geom)) { return false; }
        // Now check that it does not collide with any other items
        for (int i=0; i<ITEMS.length(); i++) {
            if (ITEMS[i]->whatsThis()==id) { continue; }
            else if (geom.intersects(ITEMS[i]->geometry())) { return false; }
        }
        return true;
    }

    LDPlugin* ItemFromID(QString ID)
    {
        for (int i=0; i<ITEMS.length(); i++) {
            if (ITEMS[i]->whatsThis()==ID) { return ITEMS[i]; }
        }
        return Q_NULLPTR;
    }

    void MovePlugin(LDPlugin* plug, QRect geom)
    {
        plug->savePluginGeometry(geom); // save the un-adjusted geometry
        plug->setGridGeometry(geomToGrid(geom)); // save the actual grid location
        plug->setGeometry( geom );
        plug->setFixedSize(geom.size()); // needed for some plugins
    }

private slots:
    void reloadPlugins(bool ForceIconUpdate = false);

    void StartItemMove(QString ID)
    {
        setupDrag(ID, "move");
    }
    void StartItemResize(QString ID)
    {
        setupDrag(ID, "resize");
    }
    void RemoveItem(QString ID)
    {
        // Special case - desktop file/dir link using the "applauncher" plugin
        if (ID.startsWith("applauncher::")) {
            QFileInfo info(ID.section("---",0,0).section("::",1,50) );
            if (info.exists() && info.absolutePath()==QDir::homePath()+"/Desktop") {
                qDebug() << "Deleting Desktop Item:" << info.absoluteFilePath();
                if (!info.isSymLink() && info.isDir()) { QProcess::startDetached("rm -r \""+info.absoluteFilePath()+"\""); }
                else { QFile::remove(info.absoluteFilePath()); } // just remove the file/symlink directly
                emit PluginRemovedByUser(ID);
                return;
            }
        }
        // Any other type of plugin
        for(int i=0; i<ITEMS.length(); i++){
            if (ITEMS[i]->whatsThis()==ID) {
                ITEMS[i]->Cleanup();
                ITEMS.takeAt(i)->deleteLater();
                break;
            }
        }
        emit PluginRemovedByUser(ID);
    }

protected:
    void focusInEvent(QFocusEvent *ev)
    {
        this->lower(); // make sure we stay on the bottom of the window stack
        QWidget::focusInEvent(ev); // do normal handling
    }
    void paintEvent(QPaintEvent*ev);

    // Need Drag and Drop functionality (internal movement)
    void dragEnterEvent(QDragEnterEvent *ev)
    {
        if (ev->mimeData()->hasFormat(MIMETYPE) ){
            ev->acceptProposedAction(); // allow this to be dropped here
        } else if (ev->mimeData()->hasUrls()) {
            ev->acceptProposedAction(); //allow this to be dropped here
        } else {
            ev->ignore();
        }
    }

    void dragMoveEvent(QDragMoveEvent *ev)
    {
        if (ev->mimeData()->hasFormat(MIMETYPE) ) {
            // Internal move/resize - Check for validity
            QString act = QString( ev->mimeData()->data(MIMETYPE) );
            LDPlugin *item = ItemFromID(act.section("::::",1,50));

            if (item!=Q_NULLPTR) {
                QRect geom = item->geometry();
                QPoint grid = posToGrid(ev->pos());
                if (act.section("::::",0,0)=="move") {
                    QPoint diff = grid - posToGrid(geom.center()); // difference in grid coords
                    geom = geomToGrid(geom); // convert to grid coords
                    geom.moveTo( (geom.topLeft()+diff) );

                    bool valid = ValidGrid(geom);
                    if (valid) {
                        // Convert to pixel coordinates and check validity again
                        geom = gridToGeom(geom); // convert back to px coords with edge matching
                        valid = ValidGeometry(act.section("::::",1,50), geom);
                    }
                    if (valid) {
                        MovePlugin(item, geom);
                        ev->acceptProposedAction();
                    } else { ev->ignore(); } // invalid location
                } else {
                    // Resize operation
                    QPoint diff = ev->pos() - (geom.center()-QPoint(1,1)); // need difference from center (pixels)
                    // Note: Use the 1x1 pixel offset to ensure that the center point is not exactly on a grid point intersection (2x2, 4x4, etc)

                    geom = geomToGrid(geom); //convert to grid coordinates now

                    if (diff.x()<0) { geom.setLeft(ev->pos().x()/GRIDSIZE); } // expanding to the left (round down)
                    else if (diff.x()>0) { geom.setRight( ev->pos().x()/GRIDSIZE); } // expanding to the right (round down)
                    if (diff.y()<0) { geom.setTop( ev->pos().y()/GRIDSIZE); } // expanding above  (round down)
                    else if (diff.y()>0) { geom.setBottom( ev->pos().y()/GRIDSIZE); } // expanding below (round down)

                    bool valid = ValidGrid(geom);
                    if (valid) {
                        // Convert to pixel coordinates and check validity again
                        geom = gridToGeom(geom); //convert back to px coords with edge matching
                        valid = ValidGeometry(act.section("::::",1,50), geom);
                    }
                    if (valid) {
                        MovePlugin(item, geom);
                        ev->acceptProposedAction();
                    } else { ev->ignore(); } // invalid location
                }
            }
        } else if (ev->mimeData()->hasUrls()) {
            ev->acceptProposedAction(); // allow this to be dropped here
        } else {
            ev->ignore();
        }
    }
	
    void dropEvent(QDropEvent *ev)
    {
        if (ev->mimeData()->hasFormat(MIMETYPE)) {
            ev->accept();
        } else if (ev->mimeData()->hasUrls()) {
            ev->accept();
            // Files getting dropped here
            QList<QUrl> urls = ev->mimeData()->urls();
            qDebug() << "DESKTOP DROP" << urls;
            QStringList files;
            QStringList dirs;
            for (int i=0; i<urls.length(); i++) {
                if (!urls[i].isLocalFile()) { continue; }
                QFileInfo info(urls[i].toLocalFile());
                if (info.isDir()) { dirs << info.absoluteFilePath(); }
                else if (info.isFile()) { files << info.absoluteFilePath(); }
            }
            if (files.size()>0 || dirs.size()>0) {
                // Copy files/folders to Desktop
                QtFileCopier *copyHander = new QtFileCopier(this);
                QtCopyDialog *copyDialog = new QtCopyDialog(copyHander,this);
                copyDialog->setMinimumDuration(100);
                copyDialog->setAutoClose(true);
                if (files.size()>0) { copyHander->copyFiles(files, QString("%1/Desktop").arg(QDir::homePath())); }
                for (int i=0;i<dirs.size();++i) {
                    copyHander->copyDirectory(dirs.at(i), QString("%1/Desktop").arg(QDir::homePath()));
                }
            }
        }
        else { ev->ignore(); }
    }
};

#endif
