/*==============================================================================

  Program: 3D Slicer

  Copyright (c) Kitware Inc.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  This file was originally developed by Jean-Christophe Fillion-Robin, Kitware Inc.
  and was partially funded by NIH grant 3P41RR013218-12S1

==============================================================================*/

// Qt includes
#include <QFileInfo>
#include <QHBoxLayout>
#include <QListView>
#include <QStandardItemModel>

// QtCore includes
#include "qSlicerAbstractCoreModule.h"
#include "qSlicerAbstractModule.h"
#include "qSlicerAbstractModuleFactoryManager.h"
#include "qSlicerModuleFactoryManager.h"

// QtGUI includes
#include "qSlicerModulesListView.h"

// --------------------------------------------------------------------------
// qSlicerModulesListViewPrivate

//-----------------------------------------------------------------------------
class qSlicerModulesListViewPrivate
{
  Q_DECLARE_PUBLIC(qSlicerModulesListView);
protected:
  qSlicerModulesListView* const q_ptr;

public:
  qSlicerModulesListViewPrivate(qSlicerModulesListView& object);
  void init();

  void updateItem(QStandardItem* item);
  void addModules();
  void removeModules();

  QStandardItem* moduleItem(const QString& moduleName)const;
  QStringList indexListToModules(const QModelIndexList& indexes)const;
  void setModulesCheckState(const QStringList& moduleNames, Qt::CheckState check);

  QStandardItemModel* ModulesListModel;
  qSlicerAbstractModuleFactoryManager* FactoryManager;
};

// --------------------------------------------------------------------------
// qSlicerModulesListViewPrivate methods

// --------------------------------------------------------------------------
qSlicerModulesListViewPrivate::qSlicerModulesListViewPrivate(qSlicerModulesListView& object)
  :q_ptr(&object)
{
  this->ModulesListModel = 0;
  this->FactoryManager = 0;
}

// --------------------------------------------------------------------------
void qSlicerModulesListViewPrivate::init()
{
  Q_Q(qSlicerModulesListView);

  this->ModulesListModel = new QStandardItemModel(q);
  q->connect(this->ModulesListModel, SIGNAL(itemChanged(QStandardItem*)),
             q, SLOT(onItemChanged(QStandardItem*)));

  q->setModel(this->ModulesListModel);
}

// --------------------------------------------------------------------------
void qSlicerModulesListViewPrivate::updateItem(QStandardItem* item)
{
  Q_Q(qSlicerModulesListView);
  QString moduleName = item->data(Qt::UserRole).toString();
  item->setCheckable(true);
  // The module is ignored, therefore it hasn't been loaded
  if (this->FactoryManager->ignoredModuleNames().contains(moduleName))
    {
    item->setForeground(q->palette().color(QPalette::Disabled, QPalette::Text));
    }
  // The module was registered, not ignored, initialized, but failed to be loaded
  else if (qobject_cast<qSlicerModuleFactoryManager*>(this->FactoryManager) &&
           !qobject_cast<qSlicerModuleFactoryManager*>(this->FactoryManager)
           ->loadedModuleNames().contains(moduleName))
    {
    item->setForeground(Qt::red);
    }
  // Loaded module
  else
    {
    item->setForeground(QBrush()); // reset color
    }
  if (this->FactoryManager == 0 ||
      this->FactoryManager->modulesToIgnore().contains(moduleName) )
    {
    item->setCheckState(Qt::Unchecked);
    }
  else
    {
    item->setCheckState(Qt::Checked);
    }
  QString text = moduleName;
  QString tooltip = moduleName;
  qSlicerAbstractCoreModule* coreModule = this->FactoryManager->moduleInstance(moduleName);
  if (coreModule)
    {
    text = coreModule->title();
    if (coreModule->dependencies().count() > 0)
      {
      tooltip += QString("(%1)").arg(coreModule->dependencies().join(", "));
      }
    }
  item->setText(text);
  item->setToolTip(tooltip);
  qSlicerAbstractModule* module =
    qobject_cast<qSlicerAbstractModule*>(coreModule);
  if (module)
    {
    // See QTBUG-20248
    bool block = this->ModulesListModel->blockSignals(true);
    item->setIcon(module->icon());
    this->ModulesListModel->blockSignals(block);
    }
}

// --------------------------------------------------------------------------
void qSlicerModulesListViewPrivate::addModules()
{
  Q_Q(qSlicerModulesListView);
  q->addModules(q->modules());
}

// --------------------------------------------------------------------------
void qSlicerModulesListViewPrivate::removeModules()
{
  this->ModulesListModel->clear();
}

// --------------------------------------------------------------------------
QStandardItem* qSlicerModulesListViewPrivate
::moduleItem(const QString& moduleName)const
{
  QModelIndex start = this->ModulesListModel->index(0, 0);
  QModelIndexList moduleIndexes = this->ModulesListModel->match(start, Qt::UserRole, moduleName);
  if (moduleIndexes.count() == 0)
    {
    return 0;
    }
  return this->ModulesListModel->itemFromIndex(moduleIndexes.at(0));
}

// --------------------------------------------------------------------------
QStringList qSlicerModulesListViewPrivate
::indexListToModules(const QModelIndexList& indexes)const
{
  QStringList modules;
  foreach(const QModelIndex& index, indexes)
    {
    modules << index.data(Qt::DisplayRole).toString();
    }
  return modules;
}

// --------------------------------------------------------------------------
void qSlicerModulesListViewPrivate
::setModulesCheckState(const QStringList& moduleNames,
                       Qt::CheckState checkState)
{
  Q_Q(qSlicerModulesListView);
  foreach(const QString& moduleName, q->modules())
    {
    QStandardItem* moduleItem = this->moduleItem(moduleName);
    if (moduleItem == 0)
      {
      continue;
      }
    if (moduleNames.contains(moduleName))
      {
      moduleItem->setCheckState(checkState);
      }
    else
      {
      moduleItem->setCheckState(
        checkState == Qt::Checked ? Qt::Unchecked : Qt::Checked);
      }
    }
}


// --------------------------------------------------------------------------
// qSlicerModulesListView methods

// --------------------------------------------------------------------------
qSlicerModulesListView::qSlicerModulesListView(QWidget* _parent)
  : Superclass(_parent)
  , d_ptr(new qSlicerModulesListViewPrivate(*this))
{
  Q_D(qSlicerModulesListView);
  d->init();
}

// --------------------------------------------------------------------------
qSlicerModulesListView::~qSlicerModulesListView()
{
}

// --------------------------------------------------------------------------
void qSlicerModulesListView::setFactoryManager(qSlicerAbstractModuleFactoryManager* factoryManager)
{
  Q_D(qSlicerModulesListView);
  if (d->FactoryManager != 0)
    {
    disconnect(d->FactoryManager, SIGNAL(moduleInstantiated(QString)),
               this, SLOT(updateModule(QString)));
    disconnect(d->FactoryManager, SIGNAL(modulesToIgnoreChanged(QStringList)),
               this, SLOT(updateModules()));
    disconnect(d->FactoryManager, SIGNAL(moduleIgnored(QString)),
               this, SLOT(updateModule(QString)));
    disconnect(d->FactoryManager, SIGNAL(moduleLoaded(QString)),
               this, SLOT(updateModule(QString)));
    disconnect(d->FactoryManager, SIGNAL(modulesInstantiated(QStringList)),
               this, SLOT(sort()));
    d->removeModules();
    }
  d->FactoryManager = factoryManager;
  if (d->FactoryManager != 0)
    {
    connect(d->FactoryManager, SIGNAL(moduleInstantiated(QString)),
            this, SLOT(updateModule(QString)));
    connect(d->FactoryManager, SIGNAL(modulesToIgnoreChanged(QStringList)),
            this, SLOT(updateModules()));
    connect(d->FactoryManager, SIGNAL(moduleIgnored(QString)),
            this, SLOT(updateModule(QString)));
    connect(d->FactoryManager, SIGNAL(moduleLoaded(QString)),
            this, SLOT(updateModule(QString)));
    connect(d->FactoryManager, SIGNAL(modulesInstantiated(QStringList)),
            this, SLOT(sort()));
    }

  this->updateModules();
}

// --------------------------------------------------------------------------
qSlicerAbstractModuleFactoryManager* qSlicerModulesListView::factoryManager()const
{
  Q_D(const qSlicerModulesListView);
  return d->FactoryManager;
}

// --------------------------------------------------------------------------
QStringList qSlicerModulesListView::modules()const
{
  Q_D(const qSlicerModulesListView);
  QStringList modules;
  if (d->FactoryManager != 0)
    {
    modules << d->FactoryManager->registeredModuleNames();
    modules << d->FactoryManager->modulesToIgnore();
    modules << d->FactoryManager->ignoredModuleNames();
    }
  modules.removeDuplicates();
  modules.sort();
  return modules;
}

// --------------------------------------------------------------------------
QStringList qSlicerModulesListView::checkedModules()const
{
  Q_D(const qSlicerModulesListView);
  QModelIndex start = d->ModulesListModel->index(0,0);
  QModelIndexList checkedModuleList =
    d->ModulesListModel->match(start, Qt::CheckStateRole, Qt::Checked, -1);
  return d->indexListToModules(checkedModuleList);
}

// --------------------------------------------------------------------------
QStringList qSlicerModulesListView::uncheckedModules()const
{
  Q_D(const qSlicerModulesListView);
  QModelIndex start = d->ModulesListModel->index(0,0);
  QModelIndexList checkedModuleList =
    d->ModulesListModel->match(start, Qt::CheckStateRole, Qt::Unchecked, -1);
  return d->indexListToModules(checkedModuleList);
}

// --------------------------------------------------------------------------
void qSlicerModulesListView::setCheckedModules(const QStringList& moduleNames)
{
  Q_D(qSlicerModulesListView);
  d->setModulesCheckState(moduleNames, Qt::Checked);
}

// --------------------------------------------------------------------------
void qSlicerModulesListView::setUncheckedModules(const QStringList& moduleNames)
{
  Q_D(qSlicerModulesListView);
  d->setModulesCheckState(moduleNames, Qt::Unchecked);
}

// --------------------------------------------------------------------------
void qSlicerModulesListView::sort()
{
  Q_D(qSlicerModulesListView);
  d->ModulesListModel->sort(0);
}

// --------------------------------------------------------------------------
void qSlicerModulesListView::addModules(const QStringList& moduleNames)
{
  foreach(const QString& moduleName, moduleNames)
    {
    this->addModule(moduleName);
    }
}

// --------------------------------------------------------------------------
void qSlicerModulesListView::addModule(const QString& moduleName)
{
  Q_D(qSlicerModulesListView);
  Q_ASSERT(d->moduleItem(moduleName) == 0);
  QStandardItem * item = new QStandardItem();
  item->setData(moduleName, Qt::UserRole);
  d->updateItem(item);
  d->ModulesListModel->appendRow(item);
}

// --------------------------------------------------------------------------
void qSlicerModulesListView::updateModules()
{
  this->updateModules(this->modules());
}

// --------------------------------------------------------------------------
void qSlicerModulesListView::updateModules(const QStringList& moduleNames)
{
  foreach(const QString& moduleName, moduleNames)
    {
    this->updateModule(moduleName);
    }
}

// --------------------------------------------------------------------------
void qSlicerModulesListView::updateModule(const QString& moduleName)
{
  Q_D(qSlicerModulesListView);
  QStandardItem * item = d->moduleItem(moduleName);
  if (item == 0)
    {
    this->addModule(moduleName);
    }
  else
    {
    d->updateItem(item);
    }
}

// --------------------------------------------------------------------------
void qSlicerModulesListView::onItemChanged(QStandardItem* item)
{
  Q_D(qSlicerModulesListView);
  QString moduleName = item->data(Qt::UserRole).toString();
  qSlicerAbstractCoreModule* module = d->FactoryManager->moduleInstance(moduleName);
  if (item->checkState() == Qt::Checked)
    {
    d->FactoryManager->removeModuleToIgnore(moduleName);
    // ensure dependencies are checked
    if (module)
      {
      foreach(const QString& dependency, module->dependencies())
        {
        d->FactoryManager->removeModuleToIgnore(dependency);
        }
      }
    }
  else
    {
    d->FactoryManager->addModuleToIgnore(moduleName);
    // ensure dependent modules are unchecked
    if (module)
      {
      foreach(const QString& dependentModule, d->FactoryManager->dependentModules(moduleName))
        {
        d->FactoryManager->addModuleToIgnore(dependentModule);
        }
      }
    }
}
