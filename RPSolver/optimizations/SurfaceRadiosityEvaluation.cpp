#include "SurfaceRadiosityEvaluation.h"
#include <QLocale>

SurfaceRadiosityEvaluation::SurfaceRadiosityEvaluation(float val, float radius,
		int photons, bool isMaxQuality, bool isValid):
 m_val(val),
 m_radius(radius),
 m_photons(photons),
 m_interval(val, radius),
 m_isMaxQuality(isMaxQuality),
 m_isValid(isValid)
{
}

Interval SurfaceRadiosityEvaluation::interval() const
{
	return m_interval;
}

bool SurfaceRadiosityEvaluation::valid() const
{
	return m_isValid;
}

float SurfaceRadiosityEvaluation::val() const
{
	return m_val;
}

float SurfaceRadiosityEvaluation::radius() const
{
	return m_radius;
}

int SurfaceRadiosityEvaluation::photons() const
{
	return m_photons;
}

bool SurfaceRadiosityEvaluation::isMaxQuality() const
{
	return m_isMaxQuality;
}


QString SurfaceRadiosityEvaluation:: info() const
{
	QLocale locale;
	return locale.toString(m_val-m_radius, 'f', 6) + ";" +
		locale.toString(m_val, 'f', 6) + ";" +
		locale.toString(m_val+m_radius, 'f', 6) + ";" +
		locale.toString(m_photons);
}

QString SurfaceRadiosityEvaluation:: infoShort() const
{
	QLocale locale;
	return locale.toString(m_val, 'f', 6);
}

