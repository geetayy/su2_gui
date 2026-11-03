#pragma once

#include <QString>
#include <QVector>

struct SU2Element
{
    int type = 0;
    QVector<int> nodes;
};

struct SU2Boundary
{
    QString name;
    QVector<SU2Element> elements;
};

struct SU2Mesh
{
    int dimension = 0;

    QVector<QVector<double>> points;

    QVector<SU2Element> volumeElements;

    QVector<SU2Boundary> boundaries;

    QString filename;

    bool isValid() const
    {
        return dimension == 2 || dimension == 3;
    }

    int elementCountByType(int type) const
    {
        int count = 0;

        for (const SU2Element &element : volumeElements)
        {
            if (element.type == type)
                ++count;
        }

        return count;
    }

    static QString elementTypeName(int type)
    {
        switch (type)
        {
        case 1:
            return "VERTEX";

        case 3:
            return "LINE";

        case 5:
            return "TRIANGLE";

        case 9:
            return "QUADRILATERAL";

        case 10:
            return "TETRAHEDRON";

        case 12:
            return "HEXAHEDRON";

        case 13:
            return "PRISM";

        case 14:
            return "PYRAMID";

        default:
            return "UNKNOWN";
        }
    }
};

struct SU2MeshLoadResult
{
    bool success = false;
    QString error;
    SU2Mesh mesh;
};

SU2MeshLoadResult loadSU2MeshFile(const QString &filename);

QString su2ElementTypeName(int type);

int su2ElementNodeCount(int type);