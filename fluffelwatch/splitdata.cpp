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
        futureSegments.push_back(segmentData);
    }

    qDebug("Loaded %d segments from file", allSegments.size());

    /* Close file and then copy the list to future segments */
    this->filename = filename;
    file.close();
}

void SplitData::saveData(const QString& filename) {
    qDebug("saving data to %s", filename.toStdString().c_str());

    /* Open given file as a text file for write only (truncates file!) */
    QFile file(filename);

    if (!file.open(QIODevice::WriteOnly)) {
        qDebug("Could not open file.");
        return;
    }

    QTextStream out(&file);

    /* Write title of run */
    out << "TITLE: " << title << "\n\n";

    /* Write a line for each segment */
    for (int i = 0; i < allSegments.size(); ++i) {
        segment data = allSegments[i];
        out << data.title << ", " << data.runtime << ", " << data.besttime << ", " << data.mission << "\n";
    }

    /* Close file */
    this->filename = filename;
    file.close();
}

QString SplitData::getTitle() const {
    return title;
}

void SplitData::setTitle(const QString& value) {
    title = value;
}

QString SplitData::getLongestSegmentTitle() const {
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
        return getSegments(list, pastSegments, lines, true);
    }

    int pastlines = 0;
    int futurelines = 0;

    /* If there are less than (lines / 2) + 1 segments left in futureSegments, then we
     * fill up with pastSegments (if possible) */
    if (futureSegments.size() < ((lines / 2) + 1)) {
        pastlines = getSegments(list, pastSegments, lines - futureSegments.size(), true);
        futurelines = getSegments(list, futureSegments, futureSegments.size());
    } else {
        /* Now there half lines left for past segments. Subtract one for the current
         * segment line */
        pastlines = getSegments(list, pastSegments, (lines-1) / 2, true);

        /* Now there are lines-1 left for the future lines */
        futurelines = getSegments(list, futureSegments, lines - pastlines - 1);
    }

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

int SplitData::split(quint64 curtime) {
    /* Splits the current segment using curtime. Returns true if possible, false
     * if there is nothing more to split. */
    if (futureSegments.size() == 0) {
        return 0;
    }

    /* Take the first item of futureSegments */
    segment splitSegment = futureSegments.takeFirst();

    /* Save the current time as run time and calculate the positive/negative improvement */
    splitSegment.ran = true;
    splitSegment.improtime = curtime - splitSegment.runtime;
    splitSegment.runtime = curtime;

    /* Put the splitted segment to the _front_ of pastSegments to reverse the order */
    pastSegments.push_front(splitSegment);

    return futureSegments.size();
}

bool SplitData::canSplit() const {
    /* Returns true if there a futureSegments left otherwise false (which means
     * that no more splits are possible) */
    return futureSegments.size() > 0;
}

bool SplitData::hasSplit() const {
    /* Returns true if there was already a split, i.e. there is an element in
     * pastSegments, otherwise false. */
    return pastSegments.size() > 0;
}

void SplitData::reset(bool merge) {
    qDebug("reset: %d past, %d future", pastSegments.size(), futureSegments.size());

    /* Merging means that we need to go through the list of pastSegments in reverse order and
     * check if the run times were better than the best times */
    if (merge) {
        qDebug("merging");
        int pastelements = pastSegments.size();
        for (int i = 0; i < pastelements; ++i) {
            segment data = pastSegments[pastelements - i - 1];

            allSegments[i].runtime = data.runtime;
            if (data.runtime > allSegments[i].besttime) {
                allSegments[i].besttime = data.runtime;
            }
        }
    }

    /* Then copy simply the items over and renew the lists */
    futureSegments = QList<segment>(allSegments);
    pastSegments.clear();

    qDebug("after reset: %d past, %d future", pastSegments.size(), futureSegments.size());
}

QString SplitData::getFilename() const {
    return filename;
}


int SplitData::getSegments(QList<segment>& toList, const QList<segment>& fromList, int lines, bool backwards) const {
    /* Empty list */
    if (fromList.isEmpty()) {
        return 0;
    }

    /* Start copying elements. From the front of the list! */
    lines = qMin(lines, fromList.size());
    for(int i = 0; i < lines; ++i) {
        int index = backwards ? lines - i - 1: i;
        toList.push_back(fromList[index]);
    }

    return lines;
}
