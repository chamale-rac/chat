#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <QTcpSocket>
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow), socket(new QTcpSocket(this)), isConnected(false) {
    ui->setupUi(this);

    messageDisplay = new QTextEdit(this);
    messageDisplay->setReadOnly(true);

    textInput = new QLineEdit(this);

    QWidget *centralWidget = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout(centralWidget);
    layout->addWidget(messageDisplay);
    layout->addWidget(textInput);
    setCentralWidget(centralWidget);

    initializeCommands();  // Initialize the commands
    connect(textInput, &QLineEdit::returnPressed, this, &MainWindow::processUserInput);
}

MainWindow::~MainWindow() {
    delete ui;
    socket->disconnectFromHost();
}

void MainWindow::initializeCommands() {
    commandMap["\\connect"] = {3, std::bind(&MainWindow::connectToServer, this, std::placeholders::_1), "<serverIP> <serverPort> <username>", false};
    commandMap["\\broadcast"] = {1, std::bind(&MainWindow::broadcastMessage, this, std::placeholders::_1), "<message>", true};
}

void MainWindow::processUserInput() {
    QString text = textInput->text();
    textInput->clear();

    messageDisplay->append("> " + text);

    if (text == "\\help") {
        displayHelp();
        return;
    }

    QStringList parts = text.split(" ");
    QString command = parts.takeFirst();

    if (commandMap.contains(command)) {
        const CommandInfo& cmdInfo = commandMap[command];
        if (parts.size() == cmdInfo.expectedArgs && cmdInfo.connectionRequired == isConnected) {
            cmdInfo.handler(parts);
        } else {
            QString connectionRequiredText = cmdInfo.connectionRequired ? "Required" : "Not Required";
            messageDisplay->append("Usage: " + cmdInfo.usage + " [Connection " + connectionRequiredText + "]");
        }
    } else {
        messageDisplay->append("Unknown command. Type \\help for command list.");
    }
}

void MainWindow::connectToServer(const QStringList &parts) {
    QString serverIp = parts[0];
    int port = parts[1].toInt();
    QString username = parts[2];

    socket->connectToHost(serverIp, port);
    if (socket->waitForConnected(3000)) {
        messageDisplay->append("SYS - Connected to " + serverIp + " on port " + QString::number(port) + " as " + username);
        isConnected = true;
    } else {
        messageDisplay->append("ERROR - Failed to connect to " + serverIp);
    }
}

void MainWindow::broadcastMessage(const QStringList &parts) {
}

void MainWindow::displayHelp() {
    QString helpText = "COMMANDS USAGE:\n";
    for (auto it = commandMap.begin(); it != commandMap.end(); ++it) {
        QString connectionRequiredText = it.value().connectionRequired ? "Required" : "Not Required";
        helpText += "    " + it.key() + " " + it.value().usage + " [Connection " + connectionRequiredText + "]\n";
    }
    helpText += "    \\help - Show this help message";
    messageDisplay->append(helpText);
}


