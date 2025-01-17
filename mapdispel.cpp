#include "mapdispel.h"
#include "./ui_mapdispel.h"
#include <QFileDialog>
#include <sstream>

MapDispel::MapDispel(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MapDispel)
{
    ui->setupUi(this);
    this->baseAddress = QUrl("http://0.0.0.0:8080/maps/");
    this->networkManager = new QNetworkAccessManager();

    //connect(this->ui->about_button, SIGNAL(triggered()), this, this->on_about_button_clicked());
    connect(ui->aboutMenuButton, SIGNAL(triggered()), this, SLOT(on_aboutMenuButton_triggered()));
}

MapDispel::~MapDispel()
{
    delete ui;
}

/**
 * @brief getSelectedDirectory Opens a file dialog where the Warcraft III maps folder is selected
 * @return The directory path to the maps folder
 */
QString getSelectedDirectory() {
    return QFileDialog::getExistingDirectory(nullptr, "Map Directory", "MapDispel");
}

/**
 * @brief MapDispel::on_openMapDirectory_clicked Opens the directory selection dialog and returns the selected directory.
 */
void MapDispel::on_openMapDirectory_clicked()
{
    this->mapDirectoryName = getSelectedDirectory();
    if(!this->mapDirectoryName.length()) {
        return;
    }
    populateTable();
}

/**
 * @brief MapDispel::populateTable Clears the map table and re-populates it with detected maps
 */
void MapDispel::populateTable(){
    QDir mapDir = QDir(this->mapDirectoryName);
    QFileInfoList mapEntries = mapDir.entryInfoList(QStringList() << "*.w3x", QDir::Files);
    if(!mapEntries.length()) {
        QMessageBox::warning(nullptr, "Warning", "No Warcraft III maps detected. Try another directory.");
        return;
    }
    ui->directoryName->setText(this->mapDirectoryName);
    ui->mapTable->setRowCount(0);
    this->mapHashes.clear();
    for (const QFileInfo &mapFile: mapEntries) {
        filePathMap.insert(mapFile.fileName(), mapFile.absoluteFilePath());
        ui->mapTable->insertRow(ui->mapTable->rowCount());
        QTableWidgetItem* mapRowItem = new QTableWidgetItem(mapFile.fileName());
        ui->mapTable->setItem(ui->mapTable->rowCount()-1,0, mapRowItem);

        QWidget *checkBoxWidget = new QWidget();
        QCheckBox *checkBox = new QCheckBox(checkBoxWidget);
        QHBoxLayout *layout = new QHBoxLayout(checkBoxWidget);
        layout->addWidget(checkBox);
        layout->setAlignment(Qt::AlignCenter);
        layout->setContentsMargins(0, 0, 0, 0);
        ui->mapTable->setCellWidget(ui->mapTable->rowCount()-1,2, checkBoxWidget);
        computeMD5(mapFile.absoluteFilePath());
    }
}

/**
 * @brief MapDispel::computeMD5 Computes the MD5 of a map
 * @param filePath Path to the file whose md5 is being computed
 */
void MapDispel::computeMD5(QString filePath) {
    QCryptographicHash crypto(QCryptographicHash::Md5);
    QFile file(filePath);
    file.open(QFile::ReadOnly);
    while(!file.atEnd()){
        crypto.addData(file.read(4096));
    }
    auto hash = QString(crypto.result().toHex()).toStdString();
    this->mapHashes.push_back(hash);
    file.close();
}

/**
 * @brief MapDispel::on_pushButton_clicked Called when the "Check Maps" button is clicked
 */
void MapDispel::on_pushButton_clicked()
{
    this->getMapInfo();
}

/**
 * @brief MapDispel::deleteFiles Deletes files
 * @param fileNames Vector of fully qualified paths to files for deletion
 */
void MapDispel::deleteFiles(const std::vector<QString> &fileNames) {
    for (const QString& filename: fileNames) {
        QFile file(filename);
        if (file.exists()) {
            if (!file.remove()) {
                QMessageBox::warning(nullptr, "Error", "Failed to delete the file.");
            }
        }
    }
}

/**
 * @brief MapDispel::getMapsForDeletion Gets the maps selected for deletion from the map table
 * @return A vector of map names that have been selected for deletion
 */
std::vector<QString> MapDispel::getMapsForDeletion() {
    std::vector<QString> deletePaths;
    for (int row = 0; row < ui->mapTable->rowCount(); ++row) {
        QWidget *checkBoxWidget = ui->mapTable->cellWidget(row, 2);
        if (checkBoxWidget) {
            QCheckBox *checkBox = qobject_cast<QCheckBox*>(checkBoxWidget->children().at(0));
            if (checkBox) {
                bool isChecked = checkBox->isChecked();
                QString filename = ui->mapTable->item(row, 0)->text();

                if (isChecked) {
                    deletePaths.push_back(this->filePathMap.value(filename));
                }
            }
        }
    }
    return deletePaths;
}

/**
 * @brief MapDispel::getMapInfo Sends a network request to FarSeer to determine the legitimacy of maps
 */
void MapDispel::getMapInfo()
{
    std::ostringstream out;
    if (!this->mapHashes.empty())
    {
        std::copy(this->mapHashes.begin(), this->mapHashes.end() - 1, std::ostream_iterator<std::string>(out, ","));
        out << this->mapHashes.back();
    }

    QNetworkRequest request;
    request.setUrl(this->baseAddress);
    QHttpMultiPart* multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);

    QHttpPart namePart;
    namePart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"name\""));
    namePart.setBody(out.str().c_str());
    multiPart->append(namePart);

    QNetworkReply* reply = this->networkManager->post(request, multiPart);
    multiPart->setParent(reply);
    QObject::connect(reply, &QNetworkReply::finished, [reply, this]() {
        if (reply->error() == QNetworkReply::NoError) {
            auto resp = reply->readAll();
            QString rawJsonString = QString::fromUtf8(resp);
            QString unescapedJsonString = rawJsonString.replace("\\\"", "");

            if (unescapedJsonString.startsWith('"') && unescapedJsonString.endsWith('"')) {
                unescapedJsonString = unescapedJsonString.mid(1, unescapedJsonString.length() - 2);
            }

            QJsonDocument jsonDoc = QJsonDocument::fromJson(unescapedJsonString.toUtf8());

            if (!jsonDoc.isArray()) {
                QMessageBox::warning(nullptr, "Error", "Failed to parse the network response. " + reply->errorString());
            }

            QJsonArray jsonArray = jsonDoc.array();
            qDebug() << jsonArray;
            this->mapStatus.push_back(unescapedJsonString);

            for (auto i=0; i < jsonArray.size(); i++) {
                auto mapStatus = jsonArray[i].toString();
                QTableWidgetItem* mapRowItem = new QTableWidgetItem(mapStatus );
                if (mapStatus == "official") {
                    mapRowItem->setForeground(QBrush(QColor(0, 128, 0)));
                } else if (mapStatus == "unknown") {
                    mapRowItem->setForeground(QBrush(QColor(255, 165, 0)));
                } else if (mapStatus == "cheat") {
                    mapRowItem->setForeground(QBrush(QColor(136, 8, 8)));
                }
                ui->mapTable->setItem(i,1, mapRowItem);
            }
        } else {
            QMessageBox::warning(nullptr, "Error", "Failed to process the network request. " + reply->errorString());
            qDebug() << "Error:" << reply->errorString();
        }
        reply->deleteLater();
    });
}

/**
 * @brief MapDispel::on_pushButton_2_clicked Called when the delete button is pressed
 * @
 */
void MapDispel::on_pushButton_2_clicked()
{
    const auto deletedFilenames = getMapsForDeletion();
    if (!deletedFilenames.size()) {
        QMessageBox::warning(nullptr, "Warning", "No maps were selected for deletion.");
        return;
    }
    deleteFiles(deletedFilenames);
    populateTable();
}

/**
 * @brief on_actionAbout_clicked Called when the "About" button is clicked
 */
void MapDispel::on_aboutMenuButton_triggered() {
    qDebug() << "asdfgh";
    QMessageBox::about(nullptr, "About", "Warcraft III is over 20 years old.\n"
                                         "Despite its age, custom map hacking remains a problem that the commnunity faces.\n"
                                         "This software is a small step in addressing the problem by giving a way of means"
                                         " to determine what maps are 'official'. The Hive and select discord servers are scraped"
                                         " for maps from official creators. This program communicates with a server that contains "
                                         " a weekly updated database of official maps and compares the files on your computer (only in the selected folder)"
                                         " against them.\n"
                                         "The only information sent to the server is the map checksums.\n"
                                         "For the source code and bug reports, visit https://github.com/ThomasThelen/MapDispel");
}
