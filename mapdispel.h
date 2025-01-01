#ifndef MAPDISPEL_H
#define MAPDISPEL_H

#include <QMainWindow>

#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QHttpMultiPart>
#include <QUrl>
#include <QJsonDocument>
#include <QJsonValue>
#include <QJsonArray>
#include <QJsonObject>
#include <QCheckBox>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QMap>

QT_BEGIN_NAMESPACE
namespace Ui {
class MapDispel;
}
QT_END_NAMESPACE

class MapDispel : public QMainWindow
{
    Q_OBJECT

public:
    MapDispel(QWidget *parent = nullptr);
    ~MapDispel();


private slots:
    void on_openMapDirectory_clicked();

    void on_pushButton_clicked();


    void on_pushButton_2_clicked();

private:
    Ui::MapDispel *ui;
    QUrl baseAddress;
    void RequestFinished(QNetworkReply *reply);
    QNetworkAccessManager* networkManager;
    void GetMapInfo();
    std::vector<std::string> GetMapInfoGet(std::vector<std::string>);
    QString ComputeMD5(QString filePath);
    std::vector<std::string> mapHashes;
    std::vector<QString> mapStatus;
    void checkSelectedCheckBoxes();
    void DeleteFile(const QString &fileName);
    QMap<QString, QString> filePathMap;
};
#endif // MAPDISPEL_H
