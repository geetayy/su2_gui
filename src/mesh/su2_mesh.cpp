#include "su2_mesh.hpp"

#include <QFile>
#include <QRegularExpression>
#include <QStringList>
#include <QTextStream>

namespace
{
int nodeCountForType(int type)
{
    switch (type)
    {
    case 1:  return 1; // VERTEX
    case 3:  return 2; // LINE
    case 5:  return 3; // TRIANGLE
    case 9:  return 4; // QUADRILATERAL
    case 10: return 4; // TETRAHEDRON
    case 12: return 8; // HEXAHEDRON
    case 13: return 6; // PRISM
    case 14: return 5; // PYRAMID
    default: return 0;
    }
}

QString elementTypeName(int type)
{
    switch (type)
    {
    case 1:  return "VERTEX";
    case 3:  return "LINE";
    case 5:  return "TRIANGLE";
    case 9:  return "QUADRILATERAL";
    case 10: return "TETRAHEDRON";
    case 12: return "HEXAHEDRON";
    case 13: return "PRISM";
    case 14: return "PYRAMID";
    default: return "UNKNOWN";
    }
}

QString cleanLine(QString line)
{
    line = line.trimmed();

    const int comment = line.indexOf('%');
    if (comment >= 0)
        line = line.left(comment).trimmed();

    return line;
}

QStringList fieldsOf(const QString &line)
{
    return line.split(
        QRegularExpression("\\s+"),
        Qt::SkipEmptyParts);
}

bool parseInt(const QString &text, int &value)
{
    bool ok = false;
    value = text.toInt(&ok);
    return ok;
}

bool parseDouble(const QString &text, double &value)
{
    bool ok = false;
    value = text.toDouble(&ok);
    return ok;
}

bool sectionLine(const QString &line,
                 const QString &name)
{
    return line.startsWith(name + "=");
}

QString sectionValue(const QString &line)
{
    const int equal = line.indexOf('=');
    if (equal < 0)
        return QString();

    return line.mid(equal + 1).trimmed();
}

bool firstIntegerAfterEquals(const QString &line, int &value)
{
    const QStringList fields = fieldsOf(sectionValue(line));

    if (fields.isEmpty())
        return false;

    return parseInt(fields.first(), value);
}
}

QString su2ElementTypeName(int type)
{
    return elementTypeName(type);
}

int su2ElementNodeCount(int type)
{
    return nodeCountForType(type);
}

SU2MeshLoadResult loadSU2MeshFile(const QString &filename)
{
    SU2MeshLoadResult result;

    QFile file(filename);

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        result.error =
            QString("Could not open mesh file: %1")
                .arg(filename);
        return result;
    }

    QTextStream stream(&file);

    SU2Mesh mesh;
    mesh.filename = filename;

    bool foundNDIME = false;
    bool foundNELEM = false;
    bool foundNPOIN = false;
    bool foundNMARK = false;

    int expectedElements = 0;
    int expectedPoints = 0;
    int expectedMarkers = 0;

    QVector<SU2Element> pendingVolumeElements;

    int lineNumber = 0;

    while (!stream.atEnd())
    {
        QString line = cleanLine(stream.readLine());
        ++lineNumber;

        if (line.isEmpty())
            continue;

        // ----------------------------------------------------
        // NDIME
        // ----------------------------------------------------
        if (sectionLine(line, "NDIME"))
        {
            int dimension = 0;

            if (!firstIntegerAfterEquals(line, dimension))
            {
                result.error =
                    QString("Invalid NDIME at line %1: '%2'")
                        .arg(lineNumber)
                        .arg(line);
                return result;
            }

            if (dimension != 2 && dimension != 3)
            {
                result.error =
                    QString("Unsupported mesh dimension %1 at line %2")
                        .arg(dimension)
                        .arg(lineNumber);
                return result;
            }

            mesh.dimension = dimension;
            foundNDIME = true;
            continue;
        }

        // ----------------------------------------------------
        // NELEM
        // ----------------------------------------------------
        if (sectionLine(line, "NELEM"))
        {
            if (!firstIntegerAfterEquals(line, expectedElements))
            {
                result.error =
                    QString("Invalid NELEM at line %1: '%2'")
                        .arg(lineNumber)
                        .arg(line);
                return result;
            }

            if (expectedElements < 0)
            {
                result.error = "Invalid negative NELEM.";
                return result;
            }

            foundNELEM = true;
            pendingVolumeElements.clear();
            pendingVolumeElements.reserve(expectedElements);

            for (int elementIndex = 0;
                 elementIndex < expectedElements;
                 ++elementIndex)
            {
                if (stream.atEnd())
                {
                    result.error =
                        QString(
                            "Unexpected end of file while reading "
                            "volume elements. Expected %1, read %2.")
                            .arg(expectedElements)
                            .arg(elementIndex);
                    return result;
                }

                QString elementLine =
                    cleanLine(stream.readLine());
                ++lineNumber;

                if (elementLine.isEmpty())
                {
                    --elementIndex;
                    continue;
                }

                const QStringList fields = fieldsOf(elementLine);

                if (fields.isEmpty())
                {
                    --elementIndex;
                    continue;
                }

                int type = 0;
                if (!parseInt(fields[0], type))
                {
                    result.error =
                        QString(
                            "Invalid element type at line %1: '%2'")
                            .arg(lineNumber)
                            .arg(elementLine);
                    return result;
                }

                const int nodeCount = nodeCountForType(type);

                if (nodeCount <= 0)
                {
                    result.error =
                        QString(
                            "Unsupported SU2 element type %1 at line %2")
                            .arg(type)
                            .arg(lineNumber);
                    return result;
                }

                // Need: type + nodeCount. Any trailing element ID is ignored.
                if (fields.size() < nodeCount + 1)
                {
                    result.error =
                        QString(
                            "Incomplete element at line %1. Type %2 "
                            "requires %3 nodes, but only %4 fields were found: %5")
                            .arg(lineNumber)
                            .arg(type)
                            .arg(nodeCount)
                            .arg(fields.size())
                            .arg(elementLine);
                    return result;
                }

                SU2Element element;
                element.type = type;
                element.nodes.reserve(nodeCount);

                for (int nodeIndex = 0;
                     nodeIndex < nodeCount;
                     ++nodeIndex)
                {
                    int node = -1;

                    if (!parseInt(fields[nodeIndex + 1], node))
                    {
                        result.error =
                            QString(
                                "Invalid node index at line %1: '%2'")
                                .arg(lineNumber)
                                .arg(elementLine);
                        return result;
                    }

                    // SU2 uses zero-based node numbering.
                    if (node < 0)
                    {
                        result.error =
                            QString(
                                "Element at line %1 references "
                                "negative node %2")
                                .arg(lineNumber)
                                .arg(node);
                        return result;
                    }

                    element.nodes.append(node);
                }

                pendingVolumeElements.append(element);
            }

            continue;
        }

        // ----------------------------------------------------
        // NPOIN
        // ----------------------------------------------------
        if (sectionLine(line, "NPOIN"))
        {
            // Some of your meshes contain e.g.:
            // NPOIN= 46417 46417
            // Only the first integer is the point count.
            if (!firstIntegerAfterEquals(line, expectedPoints))
            {
                result.error =
                    QString("Invalid NPOIN at line %1: '%2'")
                        .arg(lineNumber)
                        .arg(line);
                return result;
            }

            if (expectedPoints < 0)
            {
                result.error = "Invalid negative NPOIN.";
                return result;
            }

            foundNPOIN = true;
            mesh.points.clear();
            mesh.points.reserve(expectedPoints);

            for (int pointIndex = 0;
                 pointIndex < expectedPoints;
                 ++pointIndex)
            {
                if (stream.atEnd())
                {
                    result.error =
                        QString(
                            "Unexpected end of file while reading points. "
                            "Expected %1, read %2.")
                            .arg(expectedPoints)
                            .arg(pointIndex);
                    return result;
                }

                QString pointLine =
                    cleanLine(stream.readLine());
                ++lineNumber;

                if (pointLine.isEmpty())
                {
                    --pointIndex;
                    continue;
                }

                const QStringList fields = fieldsOf(pointLine);

                // Coordinates + point ID.
                if (fields.size() < mesh.dimension + 1)
                {
                    result.error =
                        QString(
                            "Invalid point record at line %1: '%2'")
                            .arg(lineNumber)
                            .arg(pointLine);
                    return result;
                }

                QVector<double> point;
                point.reserve(mesh.dimension);

                for (int d = 0; d < mesh.dimension; ++d)
                {
                    double coordinate = 0.0;

                    if (!parseDouble(fields[d], coordinate))
                    {
                        result.error =
                            QString(
                                "Invalid coordinate at line %1: '%2'")
                                .arg(lineNumber)
                                .arg(pointLine);
                        return result;
                    }

                    point.append(coordinate);
                }

                mesh.points.append(point);
            }

            continue;
        }

        // ----------------------------------------------------
        // NMARK
        // ----------------------------------------------------
        if (sectionLine(line, "NMARK"))
        {
            if (!firstIntegerAfterEquals(line, expectedMarkers))
            {
                result.error =
                    QString("Invalid NMARK at line %1: '%2'")
                        .arg(lineNumber)
                        .arg(line);
                return result;
            }

            if (expectedMarkers < 0)
            {
                result.error = "Invalid negative NMARK.";
                return result;
            }

            foundNMARK = true;
            mesh.boundaries.clear();
            mesh.boundaries.reserve(expectedMarkers);

            for (int markerIndex = 0;
                 markerIndex < expectedMarkers;
                 ++markerIndex)
            {
                // MARKER_TAG
                QString markerTagLine;

                while (!stream.atEnd())
                {
                    markerTagLine = cleanLine(stream.readLine());
                    ++lineNumber;
                    if (!markerTagLine.isEmpty())
                        break;
                }

                if (!sectionLine(markerTagLine, "MARKER_TAG"))
                {
                    result.error =
                        QString(
                            "Expected MARKER_TAG at line %1, found '%2'")
                            .arg(lineNumber)
                            .arg(markerTagLine);
                    return result;
                }

                SU2Boundary boundary;
                boundary.name = sectionValue(markerTagLine).trimmed();

                // MARKER_ELEMS
                QString markerElemsLine;

                while (!stream.atEnd())
                {
                    markerElemsLine = cleanLine(stream.readLine());
                    ++lineNumber;
                    if (!markerElemsLine.isEmpty())
                        break;
                }

                if (!sectionLine(markerElemsLine, "MARKER_ELEMS"))
                {
                    result.error =
                        QString(
                            "Expected MARKER_ELEMS after marker '%1' "
                            "at line %2, found '%3'")
                            .arg(boundary.name)
                            .arg(lineNumber)
                            .arg(markerElemsLine);
                    return result;
                }

                int markerElementCount = 0;

                if (!firstIntegerAfterEquals(
                        markerElemsLine,
                        markerElementCount))
                {
                    result.error =
                        QString(
                            "Invalid MARKER_ELEMS for marker '%1' at line %2")
                            .arg(boundary.name)
                            .arg(lineNumber);
                    return result;
                }

                boundary.elements.reserve(markerElementCount);

                for (int elementIndex = 0;
                     elementIndex < markerElementCount;
                     ++elementIndex)
                {
                    if (stream.atEnd())
                    {
                        result.error =
                            QString(
                                "Unexpected end of file while reading "
                                "marker '%1'.")
                                .arg(boundary.name);
                        return result;
                    }

                    QString elementLine =
                        cleanLine(stream.readLine());
                    ++lineNumber;

                    if (elementLine.isEmpty())
                    {
                        --elementIndex;
                        continue;
                    }

                    const QStringList fields = fieldsOf(elementLine);

                    if (fields.isEmpty())
                    {
                        --elementIndex;
                        continue;
                    }

                    int type = 0;

                    if (!parseInt(fields[0], type))
                    {
                        result.error =
                            QString(
                                "Invalid boundary element type at line %1: '%2'")
                            .arg(lineNumber)
                            .arg(elementLine);
                        return result;
                    }

                    const int nodeCount = nodeCountForType(type);

                    if (nodeCount <= 0)
                    {
                        result.error =
                            QString(
                                "Unsupported boundary element type %1 at line %2")
                                .arg(type)
                                .arg(lineNumber);
                        return result;
                    }

                    if (fields.size() < nodeCount + 1)
                    {
                        result.error =
                            QString(
                                "Incomplete boundary element at line %1. "
                                "Type %2 requires %3 nodes: '%4'")
                                .arg(lineNumber)
                                .arg(type)
                                .arg(nodeCount)
                                .arg(elementLine);
                        return result;
                    }

                    SU2Element element;
                    element.type = type;
                    element.nodes.reserve(nodeCount);

                    for (int nodeIndex = 0;
                         nodeIndex < nodeCount;
                         ++nodeIndex)
                    {
                        int node = -1;

                        if (!parseInt(fields[nodeIndex + 1], node))
                        {
                            result.error =
                                QString(
                                    "Invalid boundary node at line %1: '%2'")
                                    .arg(lineNumber)
                                    .arg(elementLine);
                            return result;
                        }

                        if (node < 0)
                        {
                            result.error =
                                QString(
                                    "Boundary '%1' references negative node %2 "
                                    "at line %3")
                                    .arg(boundary.name)
                                    .arg(node)
                                    .arg(lineNumber);
                            return result;
                        }

                        element.nodes.append(node);
                    }

                    boundary.elements.append(element);
                }

                mesh.boundaries.append(boundary);
            }

            continue;
        }

        // Ignore unrelated SU2 sections for Stage 1.
    }

    file.close();

    // --------------------------------------------------------
    // Mandatory section checks
    // --------------------------------------------------------
    if (!foundNDIME)
    {
        result.error = "Mesh does not contain NDIME.";
        return result;
    }

    if (!foundNELEM)
    {
        result.error = "Mesh does not contain NELEM.";
        return result;
    }

    if (!foundNPOIN)
    {
        result.error = "Mesh does not contain NPOIN.";
        return result;
    }

    // NMARK may be absent in a volume-only mesh.
    if (!foundNMARK)
        mesh.boundaries.clear();

    // --------------------------------------------------------
    // Validate volume connectivity now that NPOIN is known.
    // --------------------------------------------------------
    for (int elementIndex = 0;
         elementIndex < pendingVolumeElements.size();
         ++elementIndex)
    {
        const SU2Element &element =
            pendingVolumeElements[elementIndex];

        for (int node : element.nodes)
        {
            if (node < 0 || node >= mesh.points.size())
            {
                result.error =
                    QString(
                        "Element %1 references invalid node %2 "
                        "(number of points = %3).")
                        .arg(elementIndex)
                        .arg(node)
                        .arg(mesh.points.size());
                return result;
            }
        }
    }

    // --------------------------------------------------------
    // Validate boundary connectivity.
    // --------------------------------------------------------
    for (const SU2Boundary &boundary : mesh.boundaries)
    {
        for (const SU2Element &element : boundary.elements)
        {
            for (int node : element.nodes)
            {
                if (node < 0 || node >= mesh.points.size())
                {
                    result.error =
                        QString(
                            "Boundary '%1' references invalid node %2 "
                            "(number of points = %3).")
                            .arg(boundary.name)
                            .arg(node)
                            .arg(mesh.points.size());
                    return result;
                }
            }
        }
    }

    if (pendingVolumeElements.size() != expectedElements)
    {
        result.error =
            QString(
                "Expected %1 volume elements, parsed %2.")
                .arg(expectedElements)
                .arg(pendingVolumeElements.size());
        return result;
    }

    if (mesh.points.size() != expectedPoints)
    {
        result.error =
            QString(
                "Expected %1 points, parsed %2.")
                .arg(expectedPoints)
                .arg(mesh.points.size());
        return result;
    }

    mesh.volumeElements = std::move(pendingVolumeElements);

    result.success = true;
    result.mesh = std::move(mesh);

    return result;
}
