#include "FCErrorDialog.h"
#include "ui_FCErrorDialog.h"

FCErrorDialog::FCErrorDialog(QWidget *parent)
    : QDialog(parent)
    , _ui(new Ui::FCErrorDialog)
{
    _ui->setupUi(this);
    build();

    // Явное подключение
    connect(_ui->ok, &QToolButton::pressed, this, [this](){ close(); });
}

FCErrorDialog::~FCErrorDialog()
{
    delete _ui;
}

void FCErrorDialog::setMessage(const QString &message)
{
    if (_ui && _ui->label) {
        _ui->label->setText(message);
    }
}

int FCErrorDialog::exec()
{
    return QDialog::exec();
}

void FCErrorDialog::build()
{
    // Дополнительная настройка интерфейса (при необходимости)
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
}

void FCErrorDialog::on_buttonBox_accepted()
{
    emit ok();
    accept(); // Закрывает диалог с результатом Accepted
}
