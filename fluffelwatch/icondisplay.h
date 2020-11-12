#ifndef ICONDISPLAY_H
#define ICONDISPLAY_H

#include <QFile>
#include <QFileInfo>
#include <QPainter>
#include <QPixmap>
#include <QStringList>
#include <QTextStream>

class IconDisplay {
  public:
    IconDisplay();
    ~IconDisplay();

    /* Maximum number of icons allowed */
    static const int maxIcons;

    /* Load from a configuration file the definitions of the icons
     * and the grid size. */
    void loadFromFile(const QString &filename);

    /* Sets the states of the icons by bits (1 = on, 0 = off) */
    void setStates(const quint32 value);

    void showIcon(const quint32 pos);
    void hideIcon(const quint32 pos);
    void toggleIcon(const quint32 pos);

    void showAllIcons();
    void hideAllIcons();

    /* Paint the icons to the given rectangle */
    void paint(QPainter &painter, const QRect &rect);

  private:
    /* A list of pixmaps that represent the icons */
    QVector<QPixmap> icons;

    /* This is the grid size, rows times columns */
    int lines;
    int columns;

    /* The current state of all icons as a handy 32bit integer */
    quint32 states;
};

#endif // ICONDISPLAY_H
