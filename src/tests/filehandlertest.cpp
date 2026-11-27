#include "../core/filehandler.h"

#include <QTemporaryDir>
#include <QTest>
#include <QUrl>

#include <memory>

class FileHandlerTest : public QObject {
  Q_OBJECT

private Q_SLOTS:
  void testReadDataFileOpenFailure();
};

void FileHandlerTest::testReadDataFileOpenFailure() {
  QTemporaryDir directory;
  QVERIFY(directory.isValid());

  const QUrl url = QUrl::fromLocalFile(directory.path());
  const std::unique_ptr<Tellico::FileHandler::FileRef> fileRef(
      Tellico::FileHandler::fileRef(url, true));
  QVERIFY(fileRef->isValid());
  QVERIFY(!fileRef->open(true));

  QCOMPARE(Tellico::FileHandler::readDataFile(url, true), QByteArray());
}

QTEST_GUILESS_MAIN(FileHandlerTest)

#include "filehandlertest.moc"
