

#include "LoadMapDialog.hpp"

#include "io/IFileSys.hpp"

namespace ehb
{
    LoadMapDialog::LoadMapDialog(IFileSys& fileSys, QWidget* parent) :
        QDialog(parent)
    {
        // TODO: fix this so its a bit more dynamic
        resize(450, 600);

        layout = new QVBoxLayout(this);

        buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);

        treeWidget = new QTreeWidget();
        treeWidget->setColumnCount(2);
        treeWidget->setHeaderLabels(QStringList{"Name", "Description"});

        layout->addWidget(treeWidget);
        layout->addWidget(buttonBox);

        connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
        connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

        auto maps = fileSys.getDirectoryContents("/world/maps");

        for (const auto& map : maps)
        {
            if (auto mapdotgas = fileSys.loadGasFile(map + "/main.gas"))
            {
                QTreeWidgetItem* item = new QTreeWidgetItem;
                item->setText(0, stem(map).c_str());

                treeWidget->addTopLevelItem(item);

                auto regions = fileSys.getDirectoryContents(map + "/regions");
                for (const auto& region : regions)
                {
                    if (auto regionmaindotgas = fileSys.loadGasFile(region + "/main.gas"))
                    {
                        QTreeWidgetItem* item2 = new QTreeWidgetItem;
                        item2->setText(0, stem(region).c_str());
                        item2->setText(1, regionmaindotgas->valueAsString("region:description").c_str());
                        item->addChild(item2);
                    }
                }

                item->setExpanded(true);
            }
        }

        treeWidget->resizeColumnToContents(0);
        treeWidget->resizeColumnToContents(1);
    }

    const std::string LoadMapDialog::getSelectedMap() const
    {
        return treeWidget->selectedItems()[0]->parent()->text(0).toStdString();
    }

    const std::string LoadMapDialog::getSelectedRegion() const
    {
        return treeWidget->selectedItems()[0]->text(0).toStdString();
    }

    const std::string LoadMapDialog::getFullPathForSelectedRegion() const
    {
        return "/world/maps/" + getSelectedMap() + "/regions/" + getSelectedRegion();
    }
} // namespace ehb