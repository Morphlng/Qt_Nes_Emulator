#include "mainwindow.h"
#include "bus.h"
#include "debugger.h"
#include "ui_mainwindow.h"
#include <QDateTime>
#include <QDebug>
#include <QFileDialog>
#include <QMessageBox>
#include <QThread>

Bus Nes; // Global Variable Nes

static int read_dmc(void *, cpu_addr_t addr)
{
    return Nes.load(addr);
}

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow)
{
    WindowInit();
    InitAudio();
    OpenSound = false;

    ui->setupUi(this);
    this->setFocusPolicy(Qt::StrongFocus);
    ui->centralWidget->setFocusPolicy(Qt::NoFocus);
    ui->graphicsView->setFocusPolicy(Qt::NoFocus);
    ui->graphicsView->setScene(scene_game);

    // Bind dmc_reader
    Nes.Apu.dmc_reader(read_dmc, NULL);

    connect(ui->actionExit, &QAction::triggered, this, &MainWindow::close);
    connect(ui->ActionChooseFile, &QAction::triggered, this, &MainWindow::OnChooseFile);
    connect(ui->actionOpenDebugger, &QAction::triggered, this, [=]() {
        Debugger *debugger = new Debugger(this);
        debugger->ConnectToBus(&Nes);
        debugger->show();
    });
    connect(ui->actionOpenSound, &QAction::triggered, this, &MainWindow::ToggleSound);
    connect(ui->actionSave, &QAction::triggered, this, &MainWindow::SaveGame);
    connect(ui->actionLoad, &QAction::triggered, this, &MainWindow::LoadGame);
    connect(ui->actionReload, &QAction::triggered, this, &MainWindow::ReloadGame);

    ToggleSound(); // Turn on sound by default
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::WindowInit()
{
    scene_game = new QGraphicsScene;
    pixels = new QRgb[256 * 240];
    timer_game = NULL;
    file_path = "";
}

void MainWindow::InitAudio()
{
    if (m_audioOutput)
        delete m_audioOutput;

    QAudioFormat format;
    format.setSampleRate(44100);
    format.setChannelCount(1);
    format.setSampleSize(16);
    format.setCodec("audio/pcm");
    format.setByteOrder(QAudioFormat::LittleEndian);
    format.setSampleType(QAudioFormat::SignedInt);

    QAudioDeviceInfo dev = QAudioDeviceInfo::defaultOutputDevice();
    if (!dev.isFormatSupported(format)) {
        format = dev.nearestFormat(format);
    }

    m_audioOutput = new QAudioOutput(dev, format);
    qreal linearVolume = QAudio::convertVolume(90 / qreal(100),
                                               QAudio::LogarithmicVolumeScale,
                                               QAudio::LinearVolumeScale);
    m_audioOutput->setVolume(linearVolume);
    qAudioDevice = m_audioOutput->start();
}

void MainWindow::FCInit()
{
    // TODO: A milliseconds Timer couldn't assure 60fps
    timer_game = new QTimer(this);
    timer_game->setTimerType(Qt::PreciseTimer);
    connect(timer_game, &QTimer::timeout, this, &MainWindow::OnNewFrame);
    timer_game->start(16);
}

void MainWindow::PauseGame()
{
    if (timer_game) {
        timer_game->stop();
    }
}

void MainWindow::ResumeGame()
{
    if (timer_game) {
        timer_game->start(16);
    }
}

void MainWindow::OnChooseFile()
{
    QString filename = QFileDialog::getOpenFileName(this, "ChooseFile", "../");
    if (filename.toLower().endsWith(".nes")) {
        file_path = filename;
        if (timer_game) {
            timer_game->stop();
            delete timer_game;
            timer_game = NULL;  
        }
        scene_game->clear();

        Nes.cartridge.reset();
        if (Nes.cartridge.read_from_file(filename)) {
            Nes.reset();
            FCInit();
        } else
            file_path = "";
    } else if (filename == "") {
        return;
    } else {
        QMessageBox::warning(this, QStringLiteral("Warnings"), QStringLiteral("Invalid File"));
    }
}

void MainWindow::ReloadGame()
{
    if (file_path != "") {
        if (timer_game) {
            timer_game->stop();
            delete timer_game;
            timer_game = NULL;
        }
        scene_game->clear();

        Nes.cartridge.reset();
        if (Nes.cartridge.read_from_file(file_path)) {
            Nes.reset();
            FCInit();
        } else
            file_path = "";
    }
}

void MainWindow::OnNewFrame()
{
    do {
        Nes.clock();
    } while (!Nes.Ppu.frame_complete);
    Nes.Ppu.frame_complete = false;

    // Call the apu per frame
    Nes.Apu.end_frame();
    if(OpenSound) {
        Nes.Apu.out_count = Nes.Apu.read_samples(Nes.Apu.out_buf, BUFFER_SIZE);
        char *buf_ptr = (char *) Nes.Apu.out_buf;
        qAudioDevice->write(buf_ptr, Nes.Apu.out_count * 2);
    }

    scene_game->clear();

    for (int x = 0; x < 256; x++) {
        for (int y = 0; y < 240; y++) {
            pixels[y * 256 + x] = qRgb(Nes.Ppu.frame_data[x][y][0],
                                       Nes.Ppu.frame_data[x][y][1],
                                       Nes.Ppu.frame_data[x][y][2]);
        }
    }
    QImage img((uchar *) pixels, 256, 240, QImage::Format_ARGB32);
    QPixmap img_pixmap = QPixmap::fromImage(img);

    pixmap_lp = new QGraphicsPixmapItem;
    pixmap_lp->setPixmap(img_pixmap.scaled(512, 480, Qt::KeepAspectRatio));
    pixmap_lp->setPos(QPointF(0, 0));
    scene_game->addItem(pixmap_lp);
}

void MainWindow::ToggleSound()
{
    if (!OpenSound) {
        OpenSound = true;
        ui->actionOpenSound->setText("TurnOffSound");
    } else {
        OpenSound = false;
        ui->actionOpenSound->setText("TurnOnSound");
    }
}

// This function could create multiple directories at once
QString mkMutiDir(const QString path)
{
    QDir dir(path);
    if (dir.exists(path)) {
        return path;
    }
    QString parentDir = mkMutiDir(path.mid(0, path.lastIndexOf('/')));
    QString dirname = path.mid(path.lastIndexOf('/') + 1);
    QDir parentPath(parentDir);
    if (!dirname.isEmpty())
        parentPath.mkpath(dirname);
    return parentDir + "/" + dirname;
}

void MainWindow::SaveGame()
{
    if (file_path != "") {
        mkMutiDir("./save/" + Nes.cartridge.game_title);

        QFile file("./save/" + Nes.cartridge.game_title + "/"
                   + QDateTime::currentDateTime().toString("yyyyMMddhhmmss") + ".sav");
        file.open(QFile::WriteOnly);
        QDataStream output(&file);
        output << Nes;
        file.close();
    }
}

void MainWindow::LoadGame()
{
    if (file_path != "") {
        QString filename = QFileDialog::getOpenFileName(this, "ChooseFile", "./");
        if (filename.toLower().endsWith(".sav")) {
            QFile file(filename);
            file.open(QIODevice::ReadOnly);
            QDataStream input(&file);
            input >> Nes;
            file.close();
        } else if (filename == "")
            return;
        else {
            QMessageBox::warning(this, QStringLiteral("Warnings"), QStringLiteral("Invalid File"));
        }
    }
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    int key = event->key();
    if (Nes.controller_left.key_map.find(key) != Nes.controller_left.key_map.end()) {
        Nes.controller_left.cur_keystate[Nes.controller_left.key_map[key]] = true;
    } else if (Nes.controller_right.key_map.find(key) != Nes.controller_right.key_map.end()) {
        Nes.controller_right.cur_keystate[Nes.controller_right.key_map[key]] = true;
    }
}

void MainWindow::keyReleaseEvent(QKeyEvent *event)
{
    int key = event->key();
    if (Nes.controller_left.key_map.find(key) != Nes.controller_left.key_map.end()) {
        Nes.controller_left.cur_keystate[Nes.controller_left.key_map[key]] = false;
    } else if (Nes.controller_right.key_map.find(key) != Nes.controller_right.key_map.end()) {
        Nes.controller_right.cur_keystate[Nes.controller_right.key_map[key]] = false;
    }
}

void MainWindow::focusInEvent(QFocusEvent *event)
{
    ResumeGame();
    QMainWindow::focusInEvent(event);
}

void MainWindow::focusOutEvent(QFocusEvent *event)
{
    PauseGame();
    QMainWindow::focusOutEvent(event);
}
