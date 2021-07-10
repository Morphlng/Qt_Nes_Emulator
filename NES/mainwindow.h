#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QAudioOutput>
#include <QGraphicsPixmapItem>
#include <QGraphicsScene>
#include <QKeyEvent>
#include <QMainWindow>
#include <QTimer>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    void WindowInit();
    void InitAudio();
    void FCInit();

    ~MainWindow();

public slots:
    void OnNewFrame();
    void OnChooseFile();
    void PauseGame();
    void ResumeGame();
    void ToggleSound();
    void SaveGame();
    void LoadGame();
    void ReloadGame();

protected:
    void keyPressEvent(QKeyEvent *event);
    void keyReleaseEvent(QKeyEvent *event);
    void focusInEvent(QFocusEvent *event);
    void focusOutEvent(QFocusEvent *event);

private:
    QGraphicsScene *scene_game;
    QGraphicsPixmapItem *pixmap_lp;
    QTimer *timer_game;
    QTimer *timer_game_2;
    QTimer *timer_game_3;
    QRgb *pixels;
    QString file_path;
    int frame_interval;
    bool OpenSound;

private:
    Ui::MainWindow *ui;
    QAudioOutput *m_audioOutput = nullptr;
    QIODevice *qAudioDevice = nullptr;
};

#endif // MAINWINDOW_H
