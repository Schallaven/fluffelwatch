#include "splitdata.h"

SplitData::SplitData() {

}

SplitData::~SplitData() {

}

void SplitData::loadData(const QString& filename) {
    /* Clear all old segments */
    allSegments.clear();
    futureSegments.clear();
    pastSegments.clear();

    qDebug("loading data from %s", filename.toStdString().c_str());

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

        /* Title line */
        if (line.startsWith("TITLE:")) {
            title = line.right(line.size() - 6).trimmed();
            continue;
        }

        /* Split segment line and only process if there are four
         * elements. */
        QStringList fields = line.split(",");

        if (fields.size() != 4) {
            continue;
        }

        /* Prepare structure, fill in the fields, and add to list */
        segment segmentData;
        segmentData.title = fields.at(0);
        segmentData.runtime = fields.at(1).toLongLong();
        segmentData.besttime = fields.at(2).toLongLong();
        segmentData.mission = fields.at(3).toLongLong();

        allSegments.push_back(segmentData);
        futureSegments.enqueue(segmentData);
    }

    qDebug("Loaded %d segments from file", allSegments.size());

    /* Close file and then copy the list to future segments */
    file.close();
}

void SplitData::saveData(const QString& filename) const {
    qDebug("saving data to %s", filename.toStdString().c_str());

}

QString SplitData::getTitle() const {
    return title;
}

void SplitData::setTitle(const QString& value) {
    title = value;
}

QString SplitData::getLongestSegmentTitle() const
{
    if (allSegments.size() == 0) {
        return QString("");
    }

    /* Find longest title by iterating through all segments */
    int length = 0;
    int index = 0;

    for (int i = 0; i < allSegments.size(); ++i) {
        if (allSegments[i].title.length() > length) {
            length = allSegments[i].title.length();
            index = i;
        }
    }

    return allSegments[index].title;
}

int SplitData::getCurrentSegments(QList<SplitData::segment>& list, int lines) const {
    /* Current segment is the first row in futureSegments. If there is nothing left
     * in futureSegments, then get all the lines from pastSegments. */
    if (futureSegments.size() == 0) {
        return getSegments(list, pastSegments, lines);
    }

    /* Now there half lines left for past segments. Subtract one for the current
     * segment line */
    int pastlines = getSegments(list, pastSegments, (lines-1) / 2);

    /* Now there are lines-1 left for the future lines */
    int futurelines = getSegments(list, futureSegments, lines - pastlines - 1);

    /* Mark the current time */
    if (futurelines > 0) {
        list[pastlines].current = true;
    }

    /* Finally add the last segment from the future if it is not already added. */
    if (futureSegments.size() > futurelines) {
        list.push_back(futureSegments.last());
        futurelines++;
    }

    return pastlines + futurelines;
}

int SplitData::getSegments(QList<segment>& toList, const QList<segment>& fromList, int lines) const {
    /* Empty list */
    if (fromList.isEmpty()) {
        return 0;
    }

    /* Start copying elements. From the front of the list! */
    lines = qMin(lines, fromList.size());
    for(int i = 0; i < lines; ++i) {
        toList.push_back(fromList[i]);
    }

    return lines;
}
