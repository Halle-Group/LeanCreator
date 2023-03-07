/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of LeanCreator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "CppDocument.h"

#include <QDebug>

using namespace CPlusPlus;

Utils::FileNameList DependencyTable::filesDependingOn(const Utils::FileName &fileName) const
{
    Utils::FileNameList deps;

    int index = fileIndex.value(fileName, -1);
    if (index == -1)
        return deps;

    for (int i = 0; i < files.size(); ++i) {
        const QBitArray &bits = includeMap.at(i);

        if (bits.testBit(index))
            deps.append(files.at(i).name);
    }

    return deps;
}

Utils::FileNameList DependencyTable::allFilesDependingOnModifieds() const
{
    QSet<int> tmp;
    for( int j = 0; j < files.size(); j++ )
    {
        // i includes/depends on j
        for( int i = 0; i < files.size(); ++i ) {
            const QBitArray &bits = includeMap.at(i);

            if( bits.testBit(j) && files[j].modified > files[i].modified )
                tmp.insert(i);
        }
    }
    Utils::FileNameList res;
    for( QSet<int>::const_iterator i = tmp.begin(); i != tmp.end(); ++i )
        res.append(files[*i].name);
    return res;
}

bool DependencyTable::anyNewerDeps(const QString& path, uint ref, QString* reason) const
{
    int index = fileIndex.value(Utils::FileName::fromString(path), -1);
    if(index == -1 || index >= includeMap.size() )
        return false;

#if 0
    QList<int> toCheck = includes.value(index);
    for( int i = 0; i < toCheck.size(); i++ )
    {
        const File& f = files[toCheck[i]];
        if( f.modified > ref )
            return true;
    }
#else
    const QBitArray &bits = includeMap.at(index);
    for( int j = 0; j < files.size(); j++ )
    {
        if( j < bits.size() && bits.testBit(j) && files[j].modified > ref )
        {
            if( reason )
                *reason = files[j].name.toString();
            return true;
        }
    }
#endif
    return false;
}

void DependencyTable::build(const Snapshot &snapshot)
{
    files.clear();
    fileIndex.clear();
    includes.clear();
    includeMap.clear();

    const int documentCount = snapshot.size();
    files.resize(documentCount);
    includeMap.resize(documentCount);

    int i = 0;
    for (Snapshot::const_iterator it = snapshot.begin(); it != snapshot.end();
            ++it, ++i) {
        files[i] = File(it.key(),QFileInfo(it.key().toString()).lastModified().toTime_t());
        fileIndex[it.key()] = i;
    }

    for (int i = 0; i < files.size(); ++i) {
        const File &file = files.at(i);
        if (Document::Ptr doc = snapshot.document(file.name)) {
            QBitArray bitmap(files.size());
            QList<int> directIncludes;
            const QStringList documentIncludes = doc->includedFiles();

            foreach (const QString &includedFile, documentIncludes) {
                int index = fileIndex.value(Utils::FileName::fromString(includedFile));

                if (index == -1)
                    continue;
                else if (! directIncludes.contains(index))
                    directIncludes.append(index);

                bitmap.setBit(index, true);
            }

            includeMap[i] = bitmap;
            includes[i] = directIncludes;
        }
    }

    bool changed;

    do {
        changed = false;

        for (int i = 0; i < files.size(); ++i) {
            QBitArray bitmap = includeMap.value(i);
            QBitArray previousBitmap = bitmap;

            foreach (int includedFileIndex, includes.value(i)) {
                bitmap |= includeMap.value(includedFileIndex);
            }

            if (bitmap != previousBitmap) {
                includeMap[i] = bitmap;
                changed = true;
            }
        }
    } while (changed);
}
