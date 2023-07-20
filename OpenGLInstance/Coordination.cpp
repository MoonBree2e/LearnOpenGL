#include "Coordination.h"
#include <time.h>

using namespace glm;
CCoordChange g_coord;

CCoordChange::CCoordChange()
{
    SetEllipse();
};

CCoordChange::~CCoordChange(void)
{
}

void CCoordChange::LL_RA_formula(
    const ellipse_type& ellipse,
    double long_tud, double la_tud, double h,
    double vec[])
{
    double v;
    double lat, lon;
    lat = glm::radians(la_tud);
    lon = glm::radians(long_tud);
    double sinlat = sin(lat);
    double coslat = cos(lat);
    double sinlon = sin(lon);
    double coslon = cos(lon);

    v = ellipse.a / sqrt(1 - ellipse.ee1 * sinlat * sinlat);
    vec[0] = ((v + h) * coslat * coslon);
    vec[1] = ((v + h) * coslat * sinlon);
    vec[2] = (((1 - ellipse.ee1) * v + h) * sinlat);

    return;
}

void  CCoordChange::LL_RA_formula(
    const ellipse_type& ellipse,
    const glm::dvec3& lon_lat_alt,
    dvec3& vec)
{
    double v;
    double lat, lon;
    lat = radians(lon_lat_alt.y);
    lon = radians(lon_lat_alt.x);
    double sinlat = sin(lat);
    double coslat = cos(lat);
    double sinlon = sin(lon);
    double coslon = cos(lon);

    v = ellipse.a / sqrt(1 - ellipse.ee1 * sinlat * sinlat);
    vec.x = ((v + lon_lat_alt.z) * coslat * coslon);
    vec.y = ((v + lon_lat_alt.z) * coslat * sinlon);
    vec.z = (((1 - ellipse.ee1) * v + lon_lat_alt.z) * sinlat);
}

void  CCoordChange::LL_RA_formula(
    const ellipse_type& ellipse,
    double long_tud, double la_tud, double h,
    float vec[])
{
    double v;
    double lat, lon;
    lat = radians(la_tud);
    lon = radians(long_tud);
    double sinlat = sin(lat);
    double coslat = cos(lat);
    double sinlon = sin(lon);
    double coslon = cos(lon);

    v = ellipse.a / sqrt(1 - ellipse.ee1 * sinlat * sinlat);
    //vec[0] = (float)((v+h)*coslat*MTSinCos::getCos(lon));
    //vec[1] = (float)((v+h)*coslat*MTSinCos::getSin(lon));
    vec[0] = (float)((v + h) * coslat * coslon);
    vec[1] = (float)((v + h) * coslat * sinlon);
    vec[2] = (float)(((1 - ellipse.ee1) * v + h) * sinlat);

    return;
}

void CCoordChange::RA_LL_formula(
    const ellipse_type& ellipse,
    double x, double y, double z,
    double& lon, double& lat, double& h)
{
    lon = atan2(y, x);

    //以下使用迭代法求大地维度
    double t0, t, tOld, dt;
    dt = 1.0;
    t0 = z / sqrt(x * x + y * y);

    tOld = t0;

    double p = ellipse.c * ellipse.ee1 / sqrt(x * x + y * y);
    double k = 1 + ellipse.ee2;

    t = 0.0;
    while (dt > 0.001)
    {
        t = t0 + p * tOld / sqrt(k + tOld * tOld);
        dt = t - tOld;
        tOld = t;
    }

    lat = atan(t);

    //求出h
    double sinlat = sin(lat);
    double n = ellipse.a / sqrt(1 - ellipse.ee1 * sinlat * sinlat);
    h = sqrt(x * x + y * y) / cos(lat) - n;

    lon = degrees(lon);
    lat = degrees(lat);
}


void CCoordChange::RA_LL_formula(
    const ellipse_type& ellipse,
    const dvec3& vec,
    dvec3& lon_lat_alt)
{
    lon_lat_alt.x = atan2(vec.y, vec.x);

    //以下使用迭代法求大地维度
    double t0, t, tOld, dt;
    dt = 1.0;
    t0 = vec.z / sqrt(vec.x * vec.x + vec.y * vec.y);

    tOld = t0;

    double p = ellipse.c * ellipse.ee1 / sqrt(vec.x * vec.x + vec.y * vec.y);
    double k = 1 + ellipse.ee2;

    t = 0.0;
    while (dt > 0.001)
    {
        t = t0 + p * tOld / sqrt(k + tOld * tOld);
        dt = t - tOld;
        tOld = t;
    }

    lon_lat_alt.y = atan(t);

    //求出h
    double sinlat = sin(lon_lat_alt.y);
    double n = ellipse.a / sqrt(1 - ellipse.ee1 * sinlat * sinlat);
    lon_lat_alt.z = sqrt(vec.x * vec.x + vec.y * vec.y) / cos(lon_lat_alt.y) - n;

    lon_lat_alt.x = degrees(lon_lat_alt.x);
    lon_lat_alt.y = degrees(lon_lat_alt.y);
}

void CCoordChange::CalculateEllispse(ellipse_type& ellipse)
{
    ellipse.c = ellipse.a * ellipse.a / ellipse.b;

    ellipse.e1 = sqrt(ellipse.a * ellipse.a - ellipse.b * ellipse.b) / ellipse.a;
    ellipse.ee1 = (ellipse.a * ellipse.a - ellipse.b * ellipse.b) / (ellipse.a * ellipse.a);

    ellipse.e2 = sqrt(ellipse.a * ellipse.a - ellipse.b * ellipse.b) / ellipse.b;
    ellipse.ee2 = (ellipse.a * ellipse.a - ellipse.b * ellipse.b) / (ellipse.b * ellipse.b);;
}

void CCoordChange::SetEllipse(double a, double b) {
    /*
    WGS84
    a = 6378137.0 meters (Earth’s equatorial radius)
    b = 6356752.3142 meters (Earth’s polar radius)
    //*/

    //球模型
    WGS84_ellipse.a = a;
    WGS84_ellipse.b = b;

    //WGS84
    //WGS84_ellipse.a = 6378137;
    //WGS84_ellipse.b = 6356752.3142;

    //CSCS2000
    //WGS84_ellipse.a = 6378137;
    //WGS84_ellipse.b = 6356752.3141403558478521068615295;
    CalculateEllispse(WGS84_ellipse);
}