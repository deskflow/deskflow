// SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
// SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
#include "common/Settings.h"
#include "gui/ComputerSetupModel.h"
#include "gui/config/ServerConfig.h"
#include "gui/dialogs/ComputerSettingsDialog.h"
#include "gui/dialogs/DesktopLayoutEditor.h"
#include "gui/dialogs/ServerConfigDialog.h"
#include "gui/widgets/ComputerSetupView.h"
#include "gui/widgets/SettingsDialogButtonBox.h"
#include <QAbstractButton>
#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDir>
#include <QDoubleSpinBox>
#include <QFile>
#include <QGroupBox>
#include <QJsonDocument>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QMimeData>
#include <QPushButton>
#include <QScopeGuard>
#include <QScrollArea>
#include <QStackedWidget>
#include <QTabWidget>
#include <QTemporaryDir>
#include <QTest>
#include <QTextLayout>
#include <QTimer>
#include <QToolButton>
#include <QTranslator>

using deskflow::experience::Desktop;
using deskflow::gui::DesktopLayout;
class DesktopLayoutIntegrationTests : public QObject
{
  Q_OBJECT
private:
  const std::vector<Desktop> aligned{{"Mac", 0, 0, 1440, 900}, {"Windows", 1440, 180, 1920, 1080}};
  QByteArray configBytes() const
  {
    QFile f(Settings::settingsFile());
    if (!f.open(QIODevice::ReadOnly))
      return {};
    return f.readAll();
  }
  bool editComputerFocus(ServerConfigDialog &dialog, bool enabled)
  {
    dialog.findChild<QPushButton *>("advancedServerSettings")->click();
    dialog.findChild<QTabWidget *>("tabWidget")->setCurrentIndex(0);
    auto *view = dialog.findChild<ComputerSetupView *>();
    if (!view)
      return false;
    const auto index = view->model()->index(8 / view->model()->columnCount(), 8 % view->model()->columnCount());
    bool edited = false;
    QTimer::singleShot(0, &dialog, [&] {
      auto *child = qobject_cast<ComputerSettingsDialog *>(QApplication::activeModalWidget());
      if (!child)
        return;
      child->findChild<QCheckBox *>("chkWeakX11Focus")->setChecked(enabled);
      child->accept();
      edited = true;
    });
    QTest::mouseDClick(view->viewport(), Qt::LeftButton, Qt::NoModifier, view->visualRect(index).center());
    return edited;
  }
private Q_SLOTS:
  void rejectedComputerAliasDoesNotChangeTheDraft()
  {
    Computer computer("Windows");
    const auto original = computer;
    const auto before = configBytes();
    ComputerSettingsDialog dialog(nullptr, &computer);
    dialog.show();
    dialog.findChild<QLineEdit *>("lineNameEdit")->setText("Laptop");
    dialog.findChild<QListWidget *>("listAliases")->addItem("Laptop");
    bool warned = false;
    QTimer::singleShot(0, &dialog, [&] {
      auto *box = qobject_cast<QMessageBox *>(QApplication::activeModalWidget());
      if (box) {
        warned = true;
        box->accept();
      }
    });
    dialog.accept();
    QVERIFY(warned);
    QVERIFY(dialog.isVisible());
    QCOMPARE(computer, original);
    dialog.reject();
    QCOMPARE(computer, original);
    QCOMPARE(configBytes(), before);
  }
  void nestedComputerFocusOptionIsDiscardedWithOuterCancel()
  {
    ServerConfig config;
    const auto original = config;
    const auto before = configBytes();
    const auto key = Settings::Computer::WeakX11Focus.arg("Windows");
    const auto oldValue = Settings::value(key).toBool();
    ServerConfigDialog dialog(nullptr, config);
    dialog.show();
    QVERIFY(editComputerFocus(dialog, !oldValue));
    dialog.reject();
    QCOMPARE(config, original);
    QCOMPARE(Settings::value(key).toBool(), oldValue);
    QCOMPARE(configBytes(), before);
  }
  void computerFocusOptionCommitsAndRecalls_data()
  {
    QTest::addColumn<bool>("initial");
    QTest::newRow("enable") << false;
    QTest::newRow("disable") << true;
  }
  void computerFocusOptionCommitsAndRecalls()
  {
    QFETCH(bool, initial);
    const auto key = Settings::Computer::WeakX11Focus.arg("Windows");
    Settings::setValue(key, initial);
    Settings::save(false);
    ServerConfig config;
    const auto original = config;
    const auto before = configBytes();
    QCOMPARE(std::as_const(config).computers()[8].weakX11Focus(), initial);
    ServerConfigDialog dialog(nullptr, config);
    dialog.show();
    QVERIFY(editComputerFocus(dialog, !initial));
    auto *buttons = dialog.findChild<SettingsDialogButtonBox *>()->findChild<QDialogButtonBox *>();
    QVERIFY(buttons->button(QDialogButtonBox::Save)->isEnabled());
    QCOMPARE(config, original);
    QCOMPARE(Settings::value(key).toBool(), initial);
    QCOMPARE(configBytes(), before);
    buttons->button(QDialogButtonBox::Save)->click();
    QCOMPARE(dialog.result(), int(QDialog::Accepted));
    QVERIFY(config != original);
    QCOMPARE(std::as_const(config).computers()[8].weakX11Focus(), !initial);
    ServerConfig recalled;
    QCOMPARE(Settings::value(key).toBool(), !initial);
    QCOMPARE(recalled, config);
    auto computers = std::as_const(recalled).computers();
    ComputerSetupModel model(computers, 5, 3);
    const std::unique_ptr<QMimeData> payload(model.mimeData({model.index(1, 3)}));
    QVERIFY(
        static_cast<QAbstractItemModel *>(&model)->dropMimeData(
            payload.get(), Qt::MoveAction, -1, -1, model.index(1, 4)
        )
    );
    QCOMPARE(computers[9], std::as_const(recalled).computers()[8]);
    Computer copy = computers[9];
    ComputerSettingsDialog reopened(nullptr, &copy);
    QCOMPARE(reopened.findChild<QCheckBox *>("chkWeakX11Focus")->isChecked(), !initial);
    reopened.reject();
  }
  void resettingOuterDraftRestoresComputerFocusOption()
  {
    ServerConfig config;
    const auto original = config;
    const auto before = configBytes();
    ServerConfigDialog dialog(nullptr, config);
    dialog.show();
    QVERIFY(editComputerFocus(dialog, true));
    auto *buttons = dialog.findChild<SettingsDialogButtonBox *>()->findChild<QDialogButtonBox *>();
    QVERIFY(buttons->button(QDialogButtonBox::Save)->isEnabled());
    buttons->button(QDialogButtonBox::Reset)->click();
    QVERIFY(!buttons->button(QDialogButtonBox::Save)->isEnabled());
    QVERIFY(editComputerFocus(dialog, false));
    QVERIFY(!buttons->button(QDialogButtonBox::Save)->isEnabled());
    dialog.reject();
    QCOMPARE(config, original);
    QCOMPARE(configBytes(), before);
  }
  void readOnlyEditorKeepsInspectionAndBlocksEditingShortcuts()
  {
    DesktopLayoutEditor editor(aligned);
    editor.show();
    editor.moveDesktop("Windows", 1440, 200, false);
    const auto draft = editor.desktops();
    editor.setEditingEnabled(false);
    editor.undo();
    editor.moveDesktop("Windows", 1440, 300, false);
    editor.resizeDesktop("Windows", 1600, 900);
    QCOMPARE(editor.desktops(), draft);
    auto *picker = editor.findChild<QComboBox *>("layoutComputer");
    QVERIFY(picker->isEnabled());
    picker->setCurrentIndex(1);
    auto *details = editor.findChild<QToolButton *>("layoutAdvancedToggle");
    QVERIFY(details->isEnabled());
    details->click();
    QVERIFY(!editor.findChild<QDoubleSpinBox *>("layoutWidth")->isEnabled());
    editor.setEditingEnabled(true);
    editor.undo();
    QCOMPARE(editor.desktops(), aligned);
  }
  void arrangementIsPrimaryAndAdvancedRoundTripKeepsTheDraft()
  {
    ServerConfig config;
    const auto before = configBytes();
    ServerConfigDialog dialog(nullptr, config);
    dialog.show();
    QCoreApplication::processEvents();
    auto *editor = dialog.findChild<DesktopLayoutEditor *>("serverArrangement");
    QVERIFY(editor);
    auto *pages = dialog.findChild<QStackedWidget *>("serverConfigPages");
    auto *tabs = dialog.findChild<QTabWidget *>("tabWidget");
    QCOMPARE(pages->currentIndex(), 0);
    QVERIFY(editor->isVisible());
    QVERIFY(!tabs->isVisible());
    QVERIFY(!editor->findChild<QPushButton *>("layoutSave")->isVisible());
    const auto original = editor->desktops();
    editor->moveDesktop("Windows", 4320, 1080, false);
    const auto changed = editor->desktops();
    dialog.findChild<QPushButton *>("advancedServerSettings")->click();
    QCOMPARE(pages->currentIndex(), 1);
    QCOMPARE(tabs->currentIndex(), 1);
    tabs->setCurrentIndex(0);
    QVERIFY(dialog.findChild<QLabel *>("lblNewComputer")->isVisible());
    QCOMPARE(dialog.findChild<QLabel *>("lblNewComputer")->size(), QSize(40, 40));
    tabs->setCurrentIndex(2);
    QVERIFY(dialog.findChild<QGroupBox *>("groupExternalConfig")->isVisible());
    dialog.findChild<QPushButton *>("backToArrangement")->click();
    QCOMPARE(pages->currentIndex(), 0);
    QCOMPARE(editor->desktops(), changed);
    editor->undo();
    QCOMPARE(editor->desktops(), original);
    auto *buttons = dialog.findChild<SettingsDialogButtonBox *>()->findChild<QDialogButtonBox *>();
    QVERIFY(!buttons->button(QDialogButtonBox::Save)->isEnabled());
    QTest::keyClick(editor, Qt::Key_Escape);
    QCOMPARE(dialog.result(), int(QDialog::Rejected));
    QCOMPARE(configBytes(), before);
    QVERIFY(!config.hasDesktopLayout());
  }
  void invalidEmbeddedDraftBlocksNavigationAndResetRestoresIt()
  {
    ServerConfig config;
    QVERIFY(config.setDesktopLayout(aligned));
    const auto original = config;
    ServerConfigDialog dialog(nullptr, config);
    dialog.show();
    auto *editor = dialog.findChild<DesktopLayoutEditor *>("serverArrangement");
    auto *pages = dialog.findChild<QStackedWidget *>("serverConfigPages");
    auto *buttons = dialog.findChild<SettingsDialogButtonBox *>()->findChild<QDialogButtonBox *>();
    editor->moveDesktop("Windows", 100, 100, false);
    QVERIFY(!buttons->button(QDialogButtonBox::Save)->isEnabled());
    dialog.findChild<QPushButton *>("advancedServerSettings")->click();
    QCOMPARE(pages->currentIndex(), 0);
    QVERIFY(buttons->button(QDialogButtonBox::Reset)->isEnabled());
    buttons->button(QDialogButtonBox::Reset)->click();
    QCOMPARE(editor->desktops(), aligned);
    QVERIFY(!buttons->button(QDialogButtonBox::Save)->isEnabled());
    dialog.reject();
    QCOMPARE(config, original);
  }
  void restoringCompatibilityGridCanBeDeclinedOrCancelled()
  {
    ServerConfig config;
    QVERIFY(config.setDesktopLayout(aligned));
    const auto original = config;
    const auto before = configBytes();
    ServerConfigDialog dialog(nullptr, config);
    dialog.show();
    auto *editor = dialog.findChild<DesktopLayoutEditor *>("serverArrangement");
    dialog.findChild<QPushButton *>("advancedServerSettings")->click();
    dialog.findChild<QTabWidget *>("tabWidget")->setCurrentIndex(0);
    for (const auto response : {QMessageBox::No, QMessageBox::Yes}) {
      bool asked = false;
      QTimer::singleShot(0, &dialog, [&] {
        auto *box = qobject_cast<QMessageBox *>(QApplication::activeModalWidget());
        QVERIFY(box);
        const auto close = qScopeGuard([box] {
          if (box->isVisible())
            box->reject();
        });
        asked = true;
        box->button(response)->click();
      });
      dialog.findChild<QPushButton *>("restoreGridLayout")->click();
      QVERIFY(asked);
      if (response == QMessageBox::No)
        QCOMPARE(editor->desktops(), aligned);
      else
        QVERIFY(editor->desktops() != aligned);
      QCOMPARE(config, original);
      QCOMPARE(configBytes(), before);
    }
    dialog.findChild<QPushButton *>("backToArrangement")->click();
    auto *buttons = dialog.findChild<SettingsDialogButtonBox *>()->findChild<QDialogButtonBox *>();
    buttons->button(QDialogButtonBox::Reset)->click();
    QCOMPARE(editor->desktops(), aligned);
    buttons->button(QDialogButtonBox::Cancel)->click();
    QCOMPARE(config, original);
    QCOMPARE(configBytes(), before);
  }
  void advancedFieldsAreOptionalAndKeepFractionalGeometry()
  {
    DesktopLayoutEditor editor(aligned);
    editor.show();
    QTest::qWait(20);
    auto *toggle = editor.findChild<QToolButton *>("layoutAdvancedToggle");
    auto *panel = editor.findChild<QScrollArea *>("layoutAdvancedPanel");
    auto *x = editor.findChild<QDoubleSpinBox *>("layoutX");
    auto *width = editor.findChild<QDoubleSpinBox *>("layoutWidth");
    QVERIFY(!panel->isVisible());
    QVERIFY(!x->isVisible());
    QVERIFY(editor.findChild<QLineEdit *>("layoutDisplayName")->isVisible());
    QVERIFY(editor.findChild<QLabel *>("layoutValidation")->isVisible());
    QVERIFY(editor.findChild<QPushButton *>("layoutSave")->property("primaryAction").toBool());
    QCOMPARE(x->text(), QString("0"));
    QCOMPARE(width->text(), QString("1440"));
    toggle->setFocus();
    QTest::keyClick(toggle, Qt::Key_Space);
    QTest::qWait(20);
    QVERIFY(panel->isVisible());
    QVERIFY(x->isVisible());
    QCOMPARE(editor.desktops(), aligned);
    width->setValue(1440.5);
    QCOMPARE(editor.desktops()[0].width, 1440.5);
    QVERIFY(width->text().endsWith(".5"));
    toggle->click();
    QVERIFY(!panel->isVisible());
    QCOMPARE(editor.desktops()[0].width, 1440.5);
    editor.undo();
    QCOMPARE(editor.desktops(), aligned);
    editor.reject();
    QCOMPARE(editor.desktops(), aligned);
  }
  void displayNamesRemainLocalMetadata_data()
  {
    QTest::addColumn<QString>("label");
    QTest::newRow("chinese") << QString("工作电脑 · 办公室");
    QTest::newRow("long-english") << QString(80, 'W');
    QTest::newRow("literal-markup") << QString("<b>研发 & QA</b> \"quoted\" : = # \\");
    QTest::newRow("emoji") << QString::fromUtf8("家庭电脑 🖥️");
    QTest::newRow("empty-default") << QString();
  }
  void displayNamesRemainLocalMetadata()
  {
    QFETCH(QString, label);
    ServerConfig original;
    QVERIFY(original.setDesktopLayout(aligned));
    QString before;
    QTextStream oldStream(&before);
    oldStream << original;
    auto desktops = aligned;
    desktops[1].displayName = label.toStdString();
    ServerConfig draft = original;
    QVERIFY(draft.setDesktopLayout(desktops));
    draft.commit();
    Settings::save(false);
    ServerConfig recalled;
    QCOMPARE(recalled.desktopLayout().desktops, desktops);
    QString after;
    QTextStream newStream(&after);
    newStream << recalled;
    QCOMPARE(after, before); // Core configuration never receives label characters/escaping.
    DesktopLayoutEditor editor(desktops);
    editor.show();
    QTest::qWait(10);
    editor.findChild<QComboBox *>("layoutComputer")->setCurrentIndex(1);
    QCOMPARE(editor.findChild<QLineEdit *>("layoutDisplayName")->text(), label);
    QVERIFY(editor.findChild<QPushButton *>("layoutSave")->isEnabled());
    QVERIFY(editor.findChild<QLineEdit *>("layoutProtocolName")->text().contains("Windows"));
  }
  void invalidDisplayNameIsExplicitAndCannotCommit_data()
  {
    QTest::addColumn<QString>("label");
    QTest::newRow("newline") << QString("name\nsection: links");
    QTest::newRow("bidi") << QString("name") + QChar(0x202e) + "txt";
    QTest::newRow("nul") << QString("name") + QChar(0) + "txt";
    QTest::newRow("too-long") << QString(81, 'W');
    QTest::newRow("blank") << QString("   ");
  }
  void invalidDisplayNameIsExplicitAndCannotCommit()
  {
    QFETCH(QString, label);
    ServerConfig config;
    QVERIFY(config.setDesktopLayout(aligned));
    const auto original = config;
    auto bad = aligned;
    bad[1].displayName = label.toStdString();
    QVERIFY(!config.setDesktopLayout(bad));
    QVERIFY(config == original);
    DesktopLayoutEditor editor(bad);
    editor.show();
    QTest::qWait(10);
    QVERIFY(!editor.findChild<QPushButton *>("layoutSave")->isEnabled());
    QVERIFY(!editor.findChild<QLabel *>("layoutValidation")->text().isEmpty());
    editor.accept();
    QCOMPARE(editor.result(), 0);
  }
  void narrowEditorTextBounds_data()
  {
    QTest::addColumn<QString>("display");
    QTest::addColumn<QString>("protocol");
    QTest::newRow("chinese") << QString("办公室研发电脑 · 中文名称") << QString("Work-PC");
    QTest::newRow("long-english") << QString(80, 'W') << QString(128, 'W');
    QTest::newRow("literal-special") << QString("<b>研发 & QA</b>") << QString("Work-PC");
    QTest::newRow("invalid-too-long") << QString(81, 'W') << QString("Work-PC");
  }
  void narrowEditorTextBounds()
  {
    QFETCH(QString, display);
    QFETCH(QString, protocol);
    auto desktops = aligned;
    desktops[1].name = protocol.toStdString();
    desktops[1].displayName = display.toStdString();
    DesktopLayoutEditor editor(desktops);
    editor.show();
    editor.findChild<QComboBox *>("layoutComputer")->setCurrentIndex(1);
    editor.resize(640, 680);
    QTest::qWait(30);
    QCOMPARE(editor.width(), 640);
    auto *id = editor.findChild<QLineEdit *>("layoutProtocolName");
    QVERIFY(id->isReadOnly());
    id->selectAll();
    QCOMPARE(id->selectedText(), protocol); // Full exact ID remains copyable; no zero-width text inserted.

    for (auto *button : editor.findChildren<QAbstractButton *>())
      if (button->isVisible())
        QVERIFY(editor.rect().contains(QRect(button->mapTo(&editor, QPoint()), button->size())));
    for (auto *label : editor.findChildren<QLabel *>()) {
      if (!label->isVisible() || label->text().isEmpty())
        continue;
      QVERIFY2(
          editor.rect().contains(QRect(label->mapTo(&editor, QPoint()), label->size())), qPrintable(label->text())
      );
      if (label->wordWrap()) {
        QTextLayout text(label->text(), label->font());
        QTextOption option;
        option.setWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
        text.setTextOption(option);
        text.beginLayout();
        qreal height = 0;
        while (true) {
          auto line = text.createLine();
          if (!line.isValid())
            break;
          line.setLineWidth(label->contentsRect().width());
          height += line.height();
        }
        text.endLayout();
        QVERIFY2(label->contentsRect().height() + 2 >= height, qPrintable(label->text()));
      }
    }
  }
  void editChineseLabelUndoCancelAndRejectInvalidProtocol()
  {
    DesktopLayoutEditor editor(aligned);
    editor.show();
    editor.findChild<QComboBox *>("layoutComputer")->setCurrentIndex(1);
    auto *label = editor.findChild<QLineEdit *>("layoutDisplayName");
    label->setText("工作电脑");
    Q_EMIT label->textEdited(label->text());
    QCOMPARE(editor.desktops()[1].displayName, std::string("工作电脑"));
    editor.undo();
    QVERIFY(editor.desktops()[1].displayName.empty());
    label->setText("研发 <QA>");
    Q_EMIT label->textEdited(label->text());
    editor.reject();
    QCOMPARE(editor.desktops(), aligned);
    auto bad = aligned;
    bad[1].name = "中文协议名";
    ServerConfig config;
    QVERIFY(!config.setDesktopLayout(bad));
  }
  void renameAndRemoveCustomClientKeepGeometryAndCancelDraft()
  {
    ServerConfig config;
    QVERIFY(config.setDesktopLayout(aligned));
    const auto original = config;
    QVERIFY(config.renameLayoutClient("Windows", "Work-PC").isEmpty());
    auto renamed = aligned;
    renamed[1].name = "Work-PC";
    QCOMPARE(config.desktopLayout().desktops, renamed);
    QVERIFY(config.computerExists("Work-PC"));
    QVERIFY(!config.computerExists("Windows"));
    QVERIFY(!config.renameLayoutClient("Work-PC", "Mac").isEmpty());
    QVERIFY(!config.renameLayoutClient("Work-PC", "mac").isEmpty());
    QVERIFY(!config.renameLayoutClient("Work-PC", "bad\nsection: links").isEmpty());
    QVERIFY(!config.removeLayoutClient("Mac").isEmpty());
    QVERIFY(config.removeLayoutClient("Work-PC").isEmpty());
    QCOMPARE(config.desktopLayout().desktops.size(), size_t(1));
    QVERIFY(config.desktopLayoutError().isEmpty());
    config = original;
    QCOMPARE(config.desktopLayout().desktops, aligned);
    ServerConfigDialog dialog(nullptr, config);
    QVERIFY(dialog.findChild<QComboBox *>("layoutDevicePicker"));
    QVERIFY(dialog.findChild<QPushButton *>("renameLayoutClient"));
    QVERIFY(dialog.findChild<QPushButton *>("removeLayoutClient"));
    dialog.reject();
    QCOMPARE(config, original);
  }
  void renameUpdatesHotkeyTargetsAndRemovalBlocksDanglingReferences()
  {
    Hotkey key;
    Action action;
    action.setType(int(Action::Type::switchToComputer));
    action.setSwitchComputerName("Windows");
    action.setRestartServer(false);
    key.addAction(action);
    auto &proxy = Settings::proxy();
    proxy.beginGroup("internalConfig");
    proxy.beginWriteArray("hotkeys");
    proxy.setArrayIndex(0);
    key.saveSettings(proxy.get());
    proxy.endArray();
    proxy.endGroup();
    proxy.sync();
    ServerConfig config;
    QVERIFY(config.setDesktopLayout(aligned));
    QVERIFY(!config.removeLayoutClient("Windows").isEmpty());
    QVERIFY(config.renameLayoutClient("Windows", "Work-PC").isEmpty());
    QCOMPARE(std::as_const(config).hotkeys().first().actions().first().switchComputerName(), QString("Work-PC"));
    QVERIFY(!config.removeLayoutClient("Work-PC").isEmpty());
    config.commit();
    Settings::proxy().sync();
    ServerConfig recalled;
    QCOMPARE(recalled.desktopLayout().desktops[1].name, std::string("Work-PC"));
    QCOMPARE(std::as_const(recalled).hotkeys().first().actions().first().switchComputerName(), QString("Work-PC"));
  }
  void paletteChangeRefreshesLayoutFeedback()
  {
    DesktopLayoutEditor editor(aligned);
    editor.show();
    QCoreApplication::processEvents();
    QPalette dark = editor.palette();
    dark.setColor(QPalette::Window, QColor("#202630"));
    editor.setPalette(dark);
    QCoreApplication::processEvents();
    QVERIFY(editor.findChild<QLabel *>("layoutValidation")->styleSheet().contains("#253d36"));
    editor.moveDesktop("Windows", 100, 100, false);
    QVERIFY(editor.findChild<QLabel *>("layoutValidation")->styleSheet().contains("#432f30"));
  }
  void init()
  {
    auto &proxy = Settings::proxy();
    proxy.clear();
    proxy.sync();
    Settings::setValue(Settings::Core::ComputerName, "Mac");
    Settings::setValue(Settings::Client::LanguageSync, false);
    Settings::setValue(Settings::Security::TlsEnabled, true);
    Settings::setValue(Settings::Security::CheckPeers, true);
    Settings::setValue(Settings::Server::DesktopLayout, QString());
    proxy.beginGroup("internalConfig");
    proxy.beginWriteArray("screens");
    for (int i = 0; i < 15; ++i) {
      proxy.setArrayIndex(i);
      proxy.setValue("name", i == 7 ? "Mac" : i == 8 ? "Windows" : "");
    }
    proxy.endArray();
    proxy.endGroup();
    proxy.sync();
    Settings::save(false);
  }
  void usesIsolatedRealSettings()
  {
    QVERIFY(Settings::settingsFile().startsWith(qEnvironmentVariable("DESKFLOW_TEST_ROOT")));
    QVERIFY(Settings::tlsDir().startsWith(qEnvironmentVariable("DESKFLOW_TEST_ROOT")));
    QVERIFY(Settings::validKeys().contains(Settings::Server::DesktopLayout));
  }
  void legacyGridPreservedUntilExplicitApply()
  {
    ServerConfig config;
    QVERIFY(!config.hasDesktopLayout());
    QString text;
    QTextStream stream(&text);
    stream << config;
    QVERIFY(text.contains("right = Windows"));
    QVERIFY(text.contains("left = Mac"));
    QCOMPARE(config.desktopLayout().desktops.size(), size_t(2));
    QCOMPARE(config.desktopLayout().desktops[0].x, 2880.0);
    QCOMPARE(config.desktopLayout().desktops[1].x, 4320.0);
  }
  void commitRecallExportsReciprocalPartialEdges()
  {
    ServerConfig config;
    QVERIFY(config.setDesktopLayout(aligned));
    config.commit();
    Settings::proxy().sync();
    Settings::save(false);
    ServerConfig recalled;
    QCOMPARE(recalled.desktopLayout().desktops, aligned);
    QCOMPARE(recalled, config);
    const auto file = Settings::settingsPath() + "/core-test.conf";
    QVERIFY(recalled.save(file));
    QFile f(file);
    QVERIFY(f.open(QIODevice::ReadOnly));
    const auto output = f.readAll();
    QVERIFY(output.contains("right(20,100) = Windows(0,"));
    QVERIFY(output.contains("left(0,"));
    QVERIFY(output.contains("= Mac(20,100)"));
    QVERIFY(output.contains("section: options"));
    QCOMPARE(Settings::value(Settings::Client::LanguageSync).toBool(), false);
    QCOMPARE(Settings::value(Settings::Security::TlsEnabled).toBool(), true);
    QCOMPARE(Settings::value(Settings::Security::CheckPeers).toBool(), true);
  }
  void cancelNeverWritesConfiguration()
  {
    ServerConfig config;
    const auto original = configBytes();
    const auto originalGrid = config.desktopLayout().desktops;
    DesktopLayoutEditor editor(originalGrid);
    editor.moveDesktop("Windows", 1440, 180, false);
    editor.reject();
    QCOMPARE(editor.desktops(), originalGrid);
    QCOMPARE(configBytes(), original);
    QVERIFY(!config.hasDesktopLayout());
  }
  void invalidPlacementBlocksSaveAndUndoRestores()
  {
    DesktopLayoutEditor editor(aligned);
    editor.moveDesktop("Windows", 100, 100, false);
    QVERIFY(!editor.findChild<QPushButton *>("layoutSave")->isEnabled());
    QVERIFY(editor.findChild<QLabel *>("layoutValidation")->text().contains("overlap"));
    editor.accept();
    QCOMPARE(editor.result(), int(QDialog::Rejected));
    editor.undo();
    QCOMPARE(editor.desktops(), aligned);
    QVERIFY(editor.findChild<QPushButton *>("layoutSave")->isEnabled());
    editor.accept();
    QCOMPARE(editor.result(), int(QDialog::Accepted));
  }
  void realMouseDragSnapsAndSingleUndoRestores()
  {
    DesktopLayoutEditor editor(aligned);
    editor.show();
    QCoreApplication::processEvents();
    auto *canvas = editor.findChild<LayoutCanvas *>("layoutCanvas");
    QVERIFY(canvas);
    const auto before = canvas->desktopRect("Windows");
    const auto source = before.center().toPoint();
    QTest::mousePress(canvas, Qt::LeftButton, Qt::NoModifier, source);
    QTest::mouseMove(canvas, source + QPoint(2, 18));
    QTest::mouseRelease(canvas, Qt::LeftButton, Qt::NoModifier, source + QPoint(2, 18));
    QCOMPARE(editor.desktops()[1].x, 1440.0);
    QVERIFY(editor.desktops()[1].y > 180);
    editor.undo();
    QCOMPARE(editor.desktops(), aligned);
    QVERIFY(!editor.findChild<QPushButton *>("layoutUndo")->isEnabled());
  }
  void keyboardMoveAndResizeAreUndoable()
  {
    DesktopLayoutEditor editor(aligned);
    editor.show();
    auto *canvas = editor.findChild<LayoutCanvas *>("layoutCanvas");
    canvas->setSelected(1);
    QTest::keyClick(canvas, Qt::Key_Down, Qt::ShiftModifier);
    QCOMPARE(editor.desktops()[1].y, 181.0);
    editor.undo();
    editor.resizeDesktop("Windows", 1280, 720);
    QCOMPARE(editor.desktops()[1].width, 1280.0);
    editor.undo();
    QCOMPARE(editor.desktops(), aligned);
  }
  void editingOneFieldDoesNotRoundOtherCoordinates()
  {
    const std::vector<Desktop> fractional{{"Mac", 0, 0, 1440, 900}, {"Windows", 1440, 200.0 / 3, 1920, 1080}};
    DesktopLayoutEditor editor(fractional);
    editor.show();
    editor.findChild<QComboBox *>("layoutComputer")->setCurrentIndex(1);
    auto *width = editor.findChild<QDoubleSpinBox *>("layoutWidth");
    width->setValue(1600);
    QCOMPARE(editor.desktops()[1].y, 200.0 / 3);
    editor.undo();
    QCOMPARE(editor.desktops(), fractional);
    editor.accept();
    QCOMPARE(editor.desktops(), fractional);
  }
  void invalidSavedJsonDoesNotSilentlyFallbackOrOverwrite()
  {
    Settings::setValue(Settings::Server::DesktopLayout, "{\"version\":99}");
    ServerConfig broken;
    QVERIFY(!broken.desktopLayoutError().isEmpty());
    const auto file = Settings::settingsPath() + "/keep-old.conf";
    QFile f(file);
    QVERIFY(f.open(QIODevice::WriteOnly));
    f.write("sentinel");
    f.close();
    QVERIFY(!broken.save(file));
    QVERIFY(f.open(QIODevice::ReadOnly));
    QCOMPARE(f.readAll(), QByteArray("sentinel"));
    broken.commit();
    QCOMPARE(Settings::value(Settings::Server::DesktopLayout).toString(), QString("{\"version\":99}"));
    broken.restoreGridLayout();
    QVERIFY(broken.desktopLayoutError().isEmpty());
    QVERIFY(broken.save(file));
  }
  void unknownComputerAndInjectionAreRejected()
  {
    ServerConfig config;
    auto invalid = aligned;
    invalid[1].name = "Other";
    QVERIFY(!config.setDesktopLayout(invalid));
    invalid[1].name = "Windows\nend\nsection: options";
    QVERIFY(!config.setDesktopLayout(invalid));
    invalid = aligned;
    invalid[1].name = "mac";
    QVERIFY(!config.setDesktopLayout(invalid));
    QVERIFY(!config.hasDesktopLayout());
  }
  void restoreGridIsReversibleBeforeCommit()
  {
    ServerConfig config;
    QVERIFY(config.setDesktopLayout(aligned));
    const auto saved = config;
    config.restoreGridLayout();
    QVERIFY(!(config == saved));
    config = saved;
    QCOMPARE(config.desktopLayout().desktops, aligned);
    config.commit();
    Settings::proxy().sync();
    ServerConfig recalled;
    QVERIFY(recalled.hasDesktopLayout());
  }
  void renameServerAndAddClientPreserveGeometry()
  {
    ServerConfig config;
    QVERIFY(config.setDesktopLayout(aligned));
    Settings::setValue(Settings::Core::ComputerName, "Mac-renamed");
    config.updateServerName();
    QVERIFY(config.desktopLayoutError().isEmpty());
    QCOMPARE(config.desktopLayout().desktops[0].name, std::string("Mac-renamed"));
    config.addClient("Laptop");
    QVERIFY(config.desktopLayoutError().isEmpty());
    const auto desktops = config.desktopLayout().desktops;
    QCOMPARE(desktops.size(), size_t(3));
    QCOMPARE(desktops[1], aligned[1]);
    QCOMPARE(desktops[2].x, 3360.0);
  }
  void logicalScaleAndNegativeCoordinatesExport()
  {
    ServerConfig config;
    const std::vector<Desktop> scaled{{"Mac", -1440, -200, 1440, 900}, {"Windows", 0, 0, 2560 / 1.25, 1440 / 1.25}};
    QVERIFY(config.setDesktopLayout(scaled));
    const auto links = deskflow::experience::validateLayout(scaled).links;
    QCOMPARE(links.size(), size_t(2));
    QVERIFY(qAbs(links[0].sourceStart - 200.0 / 900 * 100) < 1e-10);
    config.commit();
    Settings::proxy().sync();
    ServerConfig recalled;
    QCOMPARE(recalled.desktopLayout().desktops, scaled);
  }
  void tinyEdgesAndOversizeMetadataAreRejected()
  {
    const std::vector<Desktop> tiny{{"Mac", 0, 0, 1440, 1000000}, {"Windows", 1440, 999999.999, 1440, 900}};
    QVERIFY(!deskflow::experience::validateLayout(tiny).valid());
    QVERIFY(!DesktopLayout::decode(QString(32769, 'x')).error.isEmpty());
    QVERIFY(!DesktopLayout::decode(
                 "{\"version\":1,\"desktops\":[{\"name\":\"Mac\",\"x\":1e300,\"y\":0,\"width\":1,\"height\":1}]}"
    )
                 .error.isEmpty());
  }
  void disconnectedLegacyDraftCanBeRepaired()
  {
    DesktopLayoutEditor editor({{"Mac", 0, 0, 1440, 900}, {"Windows", 3000, 0, 1440, 900}});
    QVERIFY(!editor.findChild<QPushButton *>("layoutSave")->isEnabled());
    editor.moveDesktop("Windows", 1450, 0, true, 16);
    QVERIFY(editor.findChild<QPushButton *>("layoutSave")->isEnabled());
    QCOMPARE(editor.desktops()[1].x, 1440.0);
  }
  void serverDialogAppliesLayoutThroughRealSaveFlow()
  {
    ServerConfig config;
    ServerConfigDialog dialog(nullptr, config);
    dialog.show();
    auto *editor = dialog.findChild<DesktopLayoutEditor *>("serverArrangement");
    QVERIFY(editor->isVisible());
    editor->moveDesktop("Windows", 4320, 1080, false);
    QVERIFY(!config.hasDesktopLayout());
    auto *buttons = dialog.findChild<SettingsDialogButtonBox *>()->findChild<QDialogButtonBox *>();
    QVERIFY(buttons->button(QDialogButtonBox::Save)->isEnabled());
    buttons->button(QDialogButtonBox::Save)->click();
    QCOMPARE(dialog.result(), int(QDialog::Accepted));
    QVERIFY(config.hasDesktopLayout());
    QCOMPARE(config.desktopLayout().desktops[1].y, 1080.0);
    ServerConfig recalled;
    QCOMPARE(recalled.desktopLayout().desktops, config.desktopLayout().desktops);
  }
  void serverDialogCancelDiscardsEmbeddedDraft()
  {
    ServerConfig config;
    const auto original = config;
    const auto before = configBytes();
    ServerConfigDialog dialog(nullptr, config);
    dialog.show();
    auto *editor = dialog.findChild<DesktopLayoutEditor *>("serverArrangement");
    QVERIFY(editor);
    editor->moveDesktop("Windows", 4320, 1080, false);
    auto *buttons = dialog.findChild<SettingsDialogButtonBox *>()->findChild<QDialogButtonBox *>();
    buttons->button(QDialogButtonBox::Cancel)->click();
    QCOMPARE(dialog.result(), int(QDialog::Rejected));
    QCOMPARE(config, original);
    QCOMPARE(configBytes(), before);
  }
  void customLayoutKeepsHotkeysAndAliasesOnCommit()
  {
    Settings::setValue(Settings::Computer::Aliases.arg("Windows"), QStringList{"win-alias"});
    Hotkey hotkey;
    KeySequence sequence;
    QVERIFY(sequence.appendKey(Qt::ControlModifier, Qt::Key_F12));
    hotkey.setKeySequence(sequence);
    Action action;
    action.setType(int(Action::Type::lockCursorToComputer));
    action.setRestartServer(false);
    hotkey.addAction(action);
    auto &proxy = Settings::proxy();
    proxy.beginGroup("internalConfig");
    proxy.beginWriteArray("hotkeys");
    proxy.setArrayIndex(0);
    hotkey.saveSettings(proxy.get());
    proxy.endArray();
    proxy.endGroup();
    proxy.sync();
    ServerConfig config;
    const auto priorHotkeys = std::as_const(config).hotkeys();
    QCOMPARE(priorHotkeys.first().text(), hotkey.text());
    QVERIFY(config.setDesktopLayout(aligned));
    config.commit();
    proxy.sync();
    ServerConfig recalled;
    QCOMPARE(std::as_const(recalled).hotkeys(), priorHotkeys);
    QCOMPARE(Settings::value(Settings::Computer::Aliases.arg("Windows")).toStringList(), QStringList{"win-alias"});
    QString output;
    QTextStream stream(&output);
    stream << recalled;
    QVERIFY(output.contains("lockCursorToScreen(toggle)"));
  }
  void externalConfigIsDraftUntilSave()
  {
    ServerConfig config;
    const auto original = configBytes();
    const auto originalPath = Settings::value(Settings::Server::ExternalConfigFile);
    ServerConfigDialog dialog(nullptr, config);
    dialog.show();
    dialog.findChild<QGroupBox *>("groupExternalConfig")->setChecked(true);
    dialog.findChild<QLineEdit *>("lineConfigFile")->setText(Settings::settingsPath() + "/other-profile.conf");
    QCOMPARE(Settings::value(Settings::Server::ExternalConfig).toBool(), false);
    QCOMPARE(Settings::value(Settings::Server::ExternalConfigFile), originalPath);
    QVERIFY(!dialog.findChild<QPushButton *>("editDesktopLayout")->isEnabled());
    dialog.findChild<SettingsDialogButtonBox *>()
        ->findChild<QDialogButtonBox *>()
        ->button(QDialogButtonBox::Cancel)
        ->click();
    QCOMPARE(configBytes(), original);
  }
};
int main(int argc, char **argv)
{
#ifdef Q_OS_WIN
  return 77; // Windows Settings isolation needs a dedicated portable build first.
#endif
  const QString root = qEnvironmentVariable("DESKFLOW_TEST_ROOT");
  if (root.isEmpty() || !QDir(root).isAbsolute() || !QDir(root).exists())
    return 2;
  QTemporaryDir isolated(root + "/layout-XXXXXX");
  if (!isolated.isValid())
    return 3;
  qputenv("XDG_CONFIG_HOME", isolated.path().toUtf8());
  qputenv("XDG_STATE_HOME", (isolated.path() + "/state").toUtf8());
  QDir().mkpath(isolated.path() + "/Deskflow");
  QDir().mkpath(isolated.path() + "/state");
  QFile config(isolated.path() + "/Deskflow/Deskflow.conf");
  if (!config.open(QIODevice::WriteOnly))
    return 4;
  config.close();
  QApplication app(argc, argv);
  app.setStyle("Fusion");
  QTranslator translator;
  const auto translation = qEnvironmentVariable("DESKFLOW_TEST_TRANSLATION");
  if (!translation.isEmpty()) {
    if (!translator.load(translation))
      return 5;
    app.installTranslator(&translator);
  }
  if (qEnvironmentVariable("DESKFLOW_TEST_THEME") == "dark") {
    QPalette palette;
    palette.setColor(QPalette::Window, QColor("#202630"));
    palette.setColor(QPalette::WindowText, QColor("#e7edf5"));
    palette.setColor(QPalette::Base, QColor("#18202c"));
    palette.setColor(QPalette::Text, QColor("#e7edf5"));
    palette.setColor(QPalette::Button, QColor("#303b4b"));
    palette.setColor(QPalette::ButtonText, QColor("#e7edf5"));
    app.setPalette(palette);
  }
  DesktopLayoutIntegrationTests tests;
  return QTest::qExec(&tests, argc, argv);
}
#include "DesktopLayoutIntegrationTests.moc"
