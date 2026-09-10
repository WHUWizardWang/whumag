#include "Login_Dialog.h"
#include "ui_Login_Dialog.h"

login_dialog::login_dialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::login_dialog)
{
    ui->setupUi(this);
}

login_dialog::~login_dialog()
{
    delete ui;
}
