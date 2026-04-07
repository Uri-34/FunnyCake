// FCYesNoDialog.cpp
#include "FCYesNoDialog.h"
#include "ui_FCYesNoDialog.h"
#include <QShortcut>
#include <QKeySequence>

FCYesNoDialog::FCYesNoDialog(QWidget *parent)
    : QDialog(parent)
    , _ui(new Ui::FCYesNoDialog)
{
    _ui->setupUi(this);
    build();
}

FCYesNoDialog::~FCYesNoDialog()
{
    delete _ui;
}

void FCYesNoDialog::setMessage(const QString &message)
{
    if (_ui && _ui->label) {
        _ui->label->setText(message);
    }
}

int FCYesNoDialog::exec()
{
    return QDialog::exec();
}

void FCYesNoDialog::build()
{
    // Убираем кнопку помощи из заголовка диалога
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    // Устанавливаем фокус на кнопку "Нет" (безопасное поведение по умолчанию)
    _ui->no->setFocus();

    // Эмуляция кнопки по умолчанию для QToolButton через обработку клавиши Enter
    // (QPushButton поддерживает setDefault(true) нативно, но QToolButton — нет)
    auto *enterShortcut = new QShortcut(QKeySequence(Qt::Key_Return), this);
    enterShortcut->setContext(Qt::WidgetWithChildrenShortcut);
    connect(enterShortcut, &QShortcut::activated, this, [this]() {
        if (this->isVisible()) {
            on_noButton_clicked(); // "Нет" как безопасный выбор по умолчанию
        }
    });

    // Подключение сигналов кнопок
    connect(_ui->yes, &QToolButton::clicked, this, &FCYesNoDialog::on_yesButton_clicked);
    connect(_ui->no,  &QToolButton::clicked, this, &FCYesNoDialog::on_noButton_clicked);
}

void FCYesNoDialog::on_yesButton_clicked()
{
    emit yes();
    accept(); // Закрывает диалог с результатом QDialog::Accepted
}

void FCYesNoDialog::on_noButton_clicked()
{
    emit no();
    reject(); // Закрывает диалог с результатом QDialog::Rejected
}
