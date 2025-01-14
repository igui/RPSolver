#include "Problem.h"
#include <QDomDocument>
#include <QXmlQuery>
#include <QXmlResultItems>
#include <QFile>
#include <QFileInfo>
#include <QXmlNodeModelIndex>
#include <QAbstractXmlNodeModel>
#include <QDebug>
#include <algorithm>
#include "conditions/LightInSurface.h"
#include "conditions/ObjectInSurface.h"
#include "conditions/DirectionalLight.h"
#include "conditions/ColorCondition.h"
#include "optimizations/SurfaceRadiosity.h"


void Problem::readScene(QFile &file, const QString& fileName)
{
	QXmlQuery query(QXmlQuery::XQuery10);
    query.setFocus(&file);
    query.setQuery("for $scene in /input/scene "
				   "	return string($scene/@path)");
 
    QStringList results;
	file.reset();
    query.evaluateTo(&results);
    int count = results.count();
	if(count == 0)
		throw std::invalid_argument("Input must have a scene definition");
    if(count > 1)
		throw std::invalid_argument("Multiple scenes are not allowed in the input");
	
	QString scenePath = results.first();
	QString absoluteScenePath = scenePath;
	if(!QFileInfo(scenePath).isAbsolute())
	{
		absoluteScenePath = QFileInfo(fileName).dir().absoluteFilePath(scenePath);
	}
	scene = Scene::createFromFile(logger, absoluteScenePath.toStdString().c_str());
	inited = true;
}

static Condition *readLightInSurface(Scene *scene, const QDomElement& element)
{
	QString id = element.attribute("id");
	QString surface = element.attribute("surface");

	if(id.isEmpty())
		throw std::logic_error("id can't be empty");
	if(surface.isEmpty())
		throw std::logic_error("surface can't be empty");

	qDebug("condition: Light in Surface id: %s, surface: %s", qPrintable(id), qPrintable(surface));

	return new LightInSurface(scene, id, surface);
}

static Condition *readObjectInSurface(Scene *scene, const QDomElement& element)
{
	QString id = element.attribute("id");
	QString surface = element.attribute("surface");
	QString surfaceVertexAIndex = element.attribute("vertexAIndex");
	QString surfaceVertexBIndex = element.attribute("vertexBIndex");
	QString surfaceVertexCIndex = element.attribute("vertexCIndex");
	QString surfaceVertexDIndex = element.attribute("vertexDIndex");

	if (id.isEmpty())
		throw std::logic_error("id can't be empty");
	if (surface.isEmpty())
		throw std::logic_error("surface can't be empty");

	if (surfaceVertexAIndex.isEmpty())
		throw std::logic_error("surface A vertex index can't be empty");
	if (surfaceVertexBIndex.isEmpty())
		throw std::logic_error("surface B vertex index can't be empty");
	if (surfaceVertexCIndex.isEmpty())
		throw std::logic_error("surface C vertex index can't be empty");
	if (surfaceVertexDIndex.isEmpty())
		throw std::logic_error("surface D vertex index can't be empty");

	bool ok;
	int vertexAIndex = surfaceVertexAIndex.toInt(&ok);
	if (!ok || vertexAIndex < 0)
		throw std::logic_error("surface A vertex index can't be empty");
	int vertexBIndex = surfaceVertexBIndex.toInt(&ok);
	if (!ok || vertexBIndex < 0)
		throw std::logic_error("surface B vertex index can't be empty");
	int vertexCIndex = surfaceVertexCIndex.toInt(&ok);
	if (!ok || vertexCIndex < 0)
		throw std::logic_error("surface C vertex index can't be empty");
	int vertexDIndex = surfaceVertexDIndex.toInt(&ok);
	if (!ok || vertexDIndex < 0)
		throw std::logic_error("surface D vertex index can't be empty");


	qDebug("condition: Object in Surface id: %s, surface: %s", qPrintable(id), qPrintable(surface));

	return new ObjectInSurface(scene, id, surface, vertexAIndex, vertexBIndex, vertexCIndex, vertexDIndex);
}

static Condition *readDirectionalLight(Scene *scene, const QDomElement& element)
{
	QString id = element.attribute("id");

	if(id.isEmpty())
		throw std::logic_error("id can't be empty");

	qDebug("condition: Directional Light id: %s", qPrintable(id));

	return new DirectionalLight(scene, id);
}

static Condition *readColorCondition(Scene *scene, const QDomElement& element)
{
	QString id = element.attribute("id");

	if (id.isEmpty())
		throw std::logic_error("id can't be empty");

	QString saturationStr = element.attribute("saturation");
	QString hueStr = element.attribute("hue");

	if (saturationStr.isEmpty())
		throw std::logic_error("saturation can't be empty");
	if (hueStr.isEmpty())
		throw std::logic_error("hue can't be empty");

	bool ok;
	float saturation = saturationStr.toFloat(&ok);
	if (!ok || saturation < 0 || saturation > 1)
		throw std::logic_error("color saturation value is invalid");
	float hue = hueStr.toFloat(&ok);
	if (!ok || hue < 0 || hue > 360)
		throw std::logic_error("color hue is invalid");
	
	qDebug("condition: Color Node id: %s", qPrintable(id));

	return new ColorCondition(id, saturation, hue);
}

static Condition *readUnknownCondition(Scene *, const QDomElement&)
{
	return NULL;
}

static Condition *readConditionElement(Scene *scene, const QDomElement& element)
{
	auto readFunc = readUnknownCondition;

	if(element.tagName() == "lightInSurface")
		readFunc = readLightInSurface;
	else if (element.tagName() == "objectInSurface")
		readFunc = readObjectInSurface;
	else if(element.tagName() == "directionalLight")
		readFunc = readDirectionalLight;
	else if (element.tagName() == "color")
		readFunc = readColorCondition;
	
	return readFunc(scene, element);
}

void Problem::readConditions(QDomDocument& xml)
{
	auto nodes = xml.documentElement().childNodes();
	for(int i = 0; i < nodes.length(); ++i)
	{
		auto conditionParentNode = nodes.at(i).toElement();
		if(conditionParentNode.tagName() != "conditions")
			continue;

		auto meshSizeStr = conditionParentNode.attribute("meshSize");
		if(meshSizeStr.isEmpty())
			throw std::logic_error("meshSize attribute must be present");
		
		auto meshSizeParts = meshSizeStr.split('x');
		meshSize.reserve(meshSizeParts.length());
		// map meshSize to proper parts
		for (auto part : meshSizeParts)
		{
			bool ok;
			auto parsed = part.trimmed().toInt(&ok);
			if (!ok || parsed <= 0)
				throw std::logic_error("meshSize attribute must be a non-negative integer");
			meshSize.append(parsed);
		};

		auto conditionNodes = conditionParentNode.childNodes();
		for(int j = 0; j < conditionNodes.length(); ++j)
		{
			auto conditionElement = conditionNodes.at(j).toElement();
			auto condition = readConditionElement(scene, conditionElement);
			if(condition)
				conditions.append(condition);
		}
		break;
	}

	if(conditions.empty())
		throw std::logic_error("at least one condition must be set");
}


void Problem::readOutputPath(const QString &fileName, QDomDocument& xml)
{
	auto nodes = xml.documentElement().childNodes();
	for(int i = 0; i < nodes.length(); ++i){
		auto outputPathNode = nodes.at(i).toElement();
		if(outputPathNode.tagName() != "output")
			continue;

		QString path = outputPathNode.attribute("path");
		if(path.isEmpty()){
			throw std::logic_error("A path attribute must be set for the output element");
		}
		
		outputDir = QDir(QFileInfo(fileName).dir().absoluteFilePath(path));
		return;
	}

	throw std::logic_error("No output path set");
}

void Problem::readOptimizationFunction(QDomDocument& xml)
{
	auto nodes = xml.documentElement().childNodes();

	for(int i = 0; i < nodes.length(); ++i)
	{
		auto objectivesNode = nodes.at(i).toElement();
		if(objectivesNode.tagName() != "objectives")
			continue;
		
		readOptimizationFunctionBaseAttrs(objectivesNode);

		auto conditionNodes = objectivesNode.childNodes();
		
		for(int j = 0; j < conditionNodes.length(); ++j)
		{
			auto maximizeRadianceNode = conditionNodes.at(j).toElement();
			readOptimizationFunctionChild(maximizeRadianceNode);
		}
	}

	if(optimizationFunction == NULL)
		throw std::logic_error("an objective must be set");
}

void Problem::readOptimizationFunctionBaseAttrs(QDomElement& objectivesNode)
{
	QString maxIterationsStr = objectivesNode.attribute("maxIterations");
	if (maxIterationsStr.isEmpty()){
		throw std::logic_error("maxIterations must be present in objectives element");
	}
	bool parseOk;
	maxIterations = maxIterationsStr.toInt(&parseOk);
	if (!parseOk){
		throw std::logic_error("maxIterations must be an integer");
	}

	auto fastEvaluationQualityStr = objectivesNode.attribute("fastEvaluationQuality");
	if (fastEvaluationQualityStr.isEmpty()){
		throw std::logic_error("fastEvaluationQuality must be present in objectives element");
	}
	fastEvaluationQuality = fastEvaluationQualityStr.toFloat(&parseOk);
	if (!parseOk){
		throw std::logic_error("fastEvaluationQuality must be a float");
	}
	if (fastEvaluationQuality < 0 || fastEvaluationQuality > 1){
		throw std::logic_error("fastEvaluationQuality must be between 0 and 1");
	}

	auto stategyStr = objectivesNode.attribute("strategy");
	if (stategyStr.isEmpty())
	{
		throw std::logic_error("strategy attribute must be present in objectives element");
	}
	if (stategyStr == "REFINE_ISOC_ON_INTERSECTION")
	{
		strategy = REFINE_ISOC_ON_INTERSECTION;
	}
	else if (stategyStr == "REFINE_ISOC_ON_END")
	{
		strategy = REFINE_ISOC_ON_END;
	}
	else if (stategyStr == "NO_REFINE_ISOC")
	{
		strategy = NO_REFINE_ISOC;
	}
	else
	{
		throw std::logic_error("Invalid value for strategy attribute");
	}
}

void Problem::readOptimizationFunctionChild(QDomElement& maximizeRadianceNode)
{
	if (maximizeRadianceNode.tagName() != "maximizeRadiance")
		throw std::logic_error("Unknown node type");

	if (optimizationFunction != NULL)
		throw std::logic_error("only an objective must be set");

	QString surface = maximizeRadianceNode.attribute("surface");
	if (surface.isEmpty())
		throw std::logic_error("surface can't be empty");

	QString maxRadiosity = maximizeRadianceNode.attribute("maxRadiosity");
	float maxRadiosityVal;
	if (maxRadiosity.isEmpty())
		maxRadiosityVal = std::numeric_limits<float>::infinity();
	else
	{
		bool ok;
		maxRadiosityVal = maxRadiosity.toFloat(&ok);
		if (!ok)
			throw std::logic_error("Invalid value for maxRadiosity");
	}

	optimizationFunction = new SurfaceRadiosity(logger, renderer, scene, surface, maxRadiosityVal);

	qDebug("objective: maximize Surface Radiosity on %s. Max value %f", qPrintable(surface), maxRadiosityVal);
}


Problem Problem::fromFile(Logger *logger, const QString& filePath, PMOptixRenderer *renderer)
{
	Problem res;

    QFile file(filePath);
	if(!QFileInfo(file).exists())
		throw std::invalid_argument(("File " + filePath + " doesn't exist").toStdString());
    file.open(QFile::ReadOnly);

	QDomDocument xml;
	{
		QString errorMsg;
		int errorLine;
		int errorColumn;
		if(!xml.setContent(&file, &errorMsg, &errorLine, &errorColumn))
		{
			QString msg = "Error while reading file: " + errorMsg;
			throw std::invalid_argument(msg.toStdString());
		}
	}
 
	file.reset();

	res.logger = logger;
	res.renderer = renderer;
	res.readScene(file, filePath);
	res.readConditions(xml);
	res.readOptimizationFunction(xml);
	res.renderer->initScene(*res.scene);
	res.readOutputPath(filePath, xml);

	return res;
}
