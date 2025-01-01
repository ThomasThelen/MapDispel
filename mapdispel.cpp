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
}

MapDispel::~MapDispel()
{
    delete ui;
}

void MapDispel::on_openMapDirectory_clicked()
{
    ui->mapTable->setRowCount(0);
    QString dirName = QFileDialog::getExistingDirectory(nullptr, "Map Directory", "123");
    ui->directoryName->setText(dirName);
    QDir mapDir = QDir(dirName);
    QFileInfoList mapEntries = mapDir.entryInfoList(QStringList() << "*.w3x", QDir::Files);

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
        ComputeMD5(mapFile.absoluteFilePath());
    }

}

QString MapDispel::ComputeMD5(QString filePath) {
    QCryptographicHash crypto(QCryptographicHash::Md5);
    QFile file(filePath);
    qDebug() << "Hashing File...";
    qDebug() << file.fileName();
    file.open(QFile::ReadOnly);
    while(!file.atEnd()){
        crypto.addData(file.read(4096));
    }
    auto hash = QString(crypto.result().toHex()).toStdString();
    qDebug() << hash;
    this->mapHashes.push_back(hash);
    file.close();
    return QString("");
}

void MapDispel::on_pushButton_clicked()
{
    this->GetMapInfo();
}

void MapDispel::DeleteFile(const QString &fileName) {
    QFile file(fileName);

    if (file.exists()) {
        if (!file.remove()) {
            // File successfully deleted
            QMessageBox::warning(nullptr, "Error", "Failed to delete the file.");
        }
    }
}

/**
 * @brief MapDispel::checkSelectedCheckBoxes
 */
void MapDispel::checkSelectedCheckBoxes() {
    for (int row = 0; row < ui->mapTable->rowCount(); ++row) {
        QWidget *checkBoxWidget = ui->mapTable->cellWidget(row, 2);
        if (checkBoxWidget) {
            QCheckBox *checkBox = qobject_cast<QCheckBox*>(checkBoxWidget->children().at(0));
            if (checkBox) {
                bool isChecked = checkBox->isChecked();
                QString filename = ui->mapTable->item(row, 0)->text();

                if (isChecked) {
                    auto fullPath = this->filePathMap.value(filename);
                    DeleteFile(fullPath);
                }
            }
        }
    }
}



void MapDispel::GetMapInfo()
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
            qDebug() << "Response received:" << resp ;
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
            qDebug() << "Error:" << reply->errorString();
        }
        reply->deleteLater();
    });
}



void MapDispel::on_pushButton_2_clicked()
{
    qDebug() << "Checking for deleted selections";
    checkSelectedCheckBoxes();
}

