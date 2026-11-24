#include "../translators/tellicoxmlreader.h"

#include <QTest>

class TellicoXmlReaderTest : public QObject {
Q_OBJECT

private Q_SLOTS:
  void testCompleteDocument();
  void testTruncatedDocument();
};

QTEST_GUILESS_MAIN(TellicoXmlReaderTest)

void TellicoXmlReaderTest::testCompleteDocument() {
  Tellico::Import::TellicoXmlReader reader{QUrl()};
  QVERIFY(reader.readNext(QByteArrayLiteral("<tellico syntaxVersion=\"11\"/>")));
  QVERIFY(reader.isComplete());
}

void TellicoXmlReaderTest::testTruncatedDocument() {
  Tellico::Import::TellicoXmlReader reader{QUrl()};
  QVERIFY(reader.readNext(QByteArrayLiteral("<tellico syntaxVersion=\"11\">")));
  QVERIFY(!reader.isComplete());
}

#include "tellicoxmlreadertest.moc"
