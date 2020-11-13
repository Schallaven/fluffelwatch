#include "icondisplay.h"

/* This is the maximum number of icons allowed */
const int IconDisplay::maxIcons = 32;


IconDisplay::IconDisplay() {
    lines = 0;
    columns = 0;
    states = 0;
}

IconDisplay::~IconDisplay() {

}

void IconDisplay::loadFromFile(const QString& filename) {
    /* Clear all icons we had before and reserve memory for the maximum number of
     * icons. */
    icons.clear();
    icons.resize(maxIcons);

    /* Get some data from this filename, i.e. the directory (important for loading
     * the icon files). */
    QFileInfo info(filename);

    qDebug("loading food data from %s", filename.toStdString().c_str());
    qDebug("Absolute path: %s", info.absolutePath().toStdString().c_str());

    /* Open given file as a text file for read only */
    QFile file(filename);

    if (!file.open(QIODevice::ReadOnly)) {
        qDebug("Could not open file.");
        return;
    }

    /* Setup a textstream and then process file line by line. */
    QTextStream in(&file);
    while (!in.atEnd()) {
        /* Read in line */
        QString line = in.readLine();

        /* Ignore empty lines and lines starting with '#' */
        if (line.startsWith('#') || (line.trimmed().size() == 0)) {
            continue;
        }

        /* Split definition lines into two parts; ignore line if not possible */
        QStringList fields = line.split("=");

        if (fields.size() != 2) {
            continue;
        }

        /* Grid size */
        if (fields[0] == "grid") {
            QStringList gridParams = fields[1].split("x");
            if (gridParams.size() != 2) {
                continue;
            }

            lines = gridParams[0].toInt();
            columns= gridParams[1].toInt();
        }

        /* Icon definition */
        if ((fields[0].toInt() > 0) && (fields[0].toInt() <= maxIcons)) {
            QPixmap icon(info.absolutePath() + "/" + fields[1]);

            if (icon.isNull()) {
                continue;
            }

            icons[fields[0].toInt()-1] = icon;
        }
    }

    file.close();
}

void IconDisplay::setStates(const quint32 value) {
    states = value;
}

void IconDisplay::showIcon(const quint32 pos) {
    if (pos < maxIcons) {
        states |= (1 << pos);
    }
}

void IconDisplay::hideIcon(const quint32 pos) {
    if (pos < maxIcons) {
        states &= ~(1 << pos);
    }
}

void IconDisplay::toggleIcon(const quint32 pos) {
    if (pos < maxIcons) {
        states ^= (1 << pos);
    }
}

void IconDisplay::showAllIcons() {
    setStates(0xFFFFFFFF);
}

void IconDisplay::hideAllIcons() {
    setStates(0);
}

void IconDisplay::paint(QPainter& painter, const QRect& rect) {
    /* If neither columns nor lines are properly set, then do not paint anything */
    if (columns == 0 || lines == 0) {
        return;
    }

    /* Calculate the size of one icon */
    int iconSize = qMin(rect.width() / columns, rect.height() / lines);
    int maxDisplay = qMin(columns * lines, maxIcons);

    /* Draw all icons */
    for(int i = 0; i < maxDisplay; ++i) {
        if (!icons[i].isNull() && (states & (1 << i))) {
            painter.drawPixmap(rect.left() + i % columns * iconSize,
                               rect.top() + i / columns * iconSize,
                               iconSize, iconSize, icons[i]);
        }
    }
}
