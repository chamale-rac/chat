#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTextEdit>
#include <QLineEdit>
#include <QVBoxLayout>
#include <QTcpSocket>
#include <functional>
#include <QMap>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    // Define CommandHandler type alias inside the class to ensure it sees 'MainWindow'
    using CommandHandler = std::function<void(const QStringList&)>;

    // Structure to hold command details
    struct CommandInfo {
        int expectedArgs;      // Number of arguments required for the command
        CommandHandler handler; // Function pointer to handle the command
        QString usage;         // Usage information for the command
        bool connectionRequired;
    };

private slots:
    void processUserInput();  // Slot to handle user input commands
    void displayHelp();       // Slot to display help information

private:
    void initializeCommands();                        // Function to initialize command map
    void handleCommands(const QString &text);         // General function to handle user commands
    void connectToServer(const QStringList &parts);   // Handle server connection
    void broadcastMessage(const QStringList &parts);  // Broadcast a message to all users

    Ui::MainWindow *ui;
    QTextEdit *messageDisplay;
    QLineEdit *textInput;
    QTcpSocket *socket;      // Socket for network communication
    bool isConnected;        // Indicator to check if the client is connected

    QMap<QString, CommandInfo> commandMap;  // Map of commands to their corresponding handlers and properties
};

#endif // MAINWINDOW_H
