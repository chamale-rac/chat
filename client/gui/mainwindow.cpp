// ./chat_client 18.118.24.164 8000 <ponele tu nombre>

#include "mainwindow.h"
#include "./ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // Create the text edit for displaying messages
    messageDisplay = new QTextEdit(this);
    messageDisplay->setReadOnly(true);

    // Create the line edit for inputting text
    textInput = new QLineEdit(this);

    // Set central widget and layout
    QWidget *centralWidget = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout(centralWidget);
    layout->addWidget(messageDisplay);
    layout->addWidget(textInput);
    setCentralWidget(centralWidget);

    // Connect the line edit's returnPressed signal to a lambda that updates the text edit
    connect(textInput, &QLineEdit::returnPressed, [this]() {
        QString text = textInput->text();
        messageDisplay->append(text);
        textInput->clear();
    });
}

MainWindow::~MainWindow()
{
    delete ui;
}
