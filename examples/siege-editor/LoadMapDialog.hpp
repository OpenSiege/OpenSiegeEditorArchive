
#pragma once

#include <string>

#include <QDialog>
#include <QVBoxLayout>
#include <QTreeWidget>
#include <QDialogButtonBox>

namespace ehb
{
    class IFileSys;
    class LoadMapDialog : public QDialog
    {
        QVBoxLayout* layout;

        QTreeWidget* treeWidget;
        QDialogButtonBox* buttonBox;

    public:

        LoadMapDialog(IFileSys& fileSys, QWidget* parent = nullptr);

        const std::string getSelectedMap() const;

        const std::string getSelectedRegion() const;

        const std::string getFullPathForSelectedRegion() const;
    };
}