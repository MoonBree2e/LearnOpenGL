#pragma once
#include "glm/glm.hpp"
#include "glm/geometric.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "glm/gtc/constants.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/quaternion.hpp"
#include "glm/gtx/quaternion.hpp"
#include "glm/gtx/euler_angles.hpp"
#include "glm/gtx/vector_angle.hpp"
#include "glm/gtx/euler_angles.hpp"
#include "glm/ext/quaternion_double.hpp"
#include "glm/gtx/matrix_decompose.hpp"
#include <math.h>

#define GLOBAL_HEIGHT  (1024 * 1024)  //Carto����ϵ�ĸ߶�
#define GLOBAL_WIDTH   (2048 * 1024)  //Carto����ϵ�Ŀ��

using GeoPosition = glm::dvec3;      //����λ�ã���lon lat alt

class CCoordChange {
    typedef struct Etype
    {
        double a;  //���򳤰���
        double b;  //����̰���
        double c;
        double e1;  //�����һ������e
        double ee1;	//�����һ������e*e
        double e2;	//����ڶ�������e
        double ee2;	//����ڶ�������e*e
    } ellipse_type;

public:
    CCoordChange();
    ~CCoordChange(void);

protected:
    ellipse_type  WGS84_ellipse;//������ģ��
private:

    //��γ������ת��Ϊֱ�����깫ʽ
    /************************************************************************
    lon: ���ȣ���x��ļн�
    lat: γ�ȣ���xyƽ��ļн�
    ************************************************************************/
    void  LL_RA_formula(
        const ellipse_type& ellipse,
        const GeoPosition& lon_lat_alt,
        glm::dvec3& vec);

    void  LL_RA_formula(
        const ellipse_type& ellipse,
        double long_tud, double la_tud, double h,
        double vec[]);

    void  LL_RA_formula(
        const ellipse_type& ellipse,
        double long_tud, double la_tud, double h,
        float vec[]);

    //ֱ������-����γ�ȴ�����깫ʽ
    void RA_LL_formula(
        const ellipse_type& ellipse,
        const float vec[],
        double& lon, double& lat, double& h)
    {
        RA_LL_formula(ellipse, vec[0], vec[1], vec[2], lon, lat, h);
    }

    //ֱ������-����γ�ȴ�����깫ʽ
    void RA_LL_formula(
        const ellipse_type& ellipse,
        const glm::dvec3& vec,
        GeoPosition& lon_lat_alt);

    void RA_LL_formula(
        const ellipse_type& ellipse,
        const double vec[],
        double& lon, double& lat, double& h)
    {
        RA_LL_formula(ellipse, vec[0], vec[1], vec[2], lon, lat, h);
    }

    //ֱ������-����γ�ȴ�����깫ʽ
    void RA_LL_formula(
        const ellipse_type& ellipse,
        double x, double y, double z,
        double& lon, double& lat, double& h);

    //���ݳ��̰��ᣬ����������������������
    void CalculateEllispse(ellipse_type& ellipse);



public:
    void SetEllipse(double a = 6378137, double b = 6378137);
    //WGS84�ľ�γ��-ֱ�ǣ�����Ƕȡ�
    void LongLat2GlobalCoord(const GeoPosition& lon_lat_alt, glm::dvec3& vec)
    {
        LL_RA_formula(WGS84_ellipse, lon_lat_alt, vec);
    }
    // WGS84�ľ�γ��-ֱ�ǣ�����Ƕȡ�
    void LongLat2GlobalCoord(double lon, double lat, double h, glm::dvec3& vec) {
        LL_RA_formula(WGS84_ellipse, lon, lat, h, glm::value_ptr(vec));
    }
    //WGS84�ľ�γ��-ֱ�ǣ�����Ƕȡ�
    void LongLat2GlobalCoord(double lon, double lat, double h, double* vec)
    {
        LL_RA_formula(WGS84_ellipse, lon, lat, h, vec);
    }

    //WGS84�ľ�γ��-ֱ�ǣ�����Ƕȡ�
    void LongLat2GlobalCoord(double lon, double lat, double h, float* vec)
    {
        LL_RA_formula(WGS84_ellipse, lon, lat, h, vec);
    }

    void LongLat2GlobalCoord(const double lon_lat_h[3], float* vec)
    {
        LL_RA_formula(WGS84_ellipse, lon_lat_h[0], lon_lat_h[1], lon_lat_h[2], vec);
    }

    void LongLat2GlobalCoord(const double lon_lat_h[3], double* vec)
    {
        LL_RA_formula(WGS84_ellipse, lon_lat_h[0], lon_lat_h[1], lon_lat_h[2], vec);
    }

    //ֱ��->WGS84�ľ�γ�ȣ�����Ƕ�(-180:180, -90:90, h)
    void GlobalCoord2LongLat(const glm::dvec3& vec, GeoPosition& lon_lat_alt)
    {
        RA_LL_formula(WGS84_ellipse, vec, lon_lat_alt);
    }

    //ֱ��->WGS84�ľ�γ�ȣ�����Ƕ�(-180:180, -90:90, h)
    void GlobalCoord2LongLat(const glm::dvec3& vec, double& lon, double& lat, double& h) {
        RA_LL_formula(WGS84_ellipse, glm::value_ptr(vec), lon, lat, h);
    }

    void GlobalCoord2LongLat(const double vec[], double& lon, double& lat, double& h)
    {
        RA_LL_formula(WGS84_ellipse, vec, lon, lat, h);
    }

    void GlobalCoord2LongLat(const float vec[], double& lon, double& lat, double& h)
    {
        RA_LL_formula(WGS84_ellipse, vec, lon, lat, h);
    }

    void GlobalCoord2LongLat(const double vec[], double lon_lat_h[])
    {
        RA_LL_formula(WGS84_ellipse, vec, lon_lat_h[0], lon_lat_h[1], lon_lat_h[2]);
    }

    void GlobalCoord2LongLat(const float vec[], double lon_lat_h[])
    {
        RA_LL_formula(WGS84_ellipse, vec, lon_lat_h[0], lon_lat_h[1], lon_lat_h[2]);
    }

    void CartoCoord2LongLat(double x, double y, double& longitude, double& latidute)
    {
        longitude = x / GLOBAL_WIDTH * 360.0 - 180;
        latidute = y / GLOBAL_HEIGHT * 180.0 - 90;
    }

    void LongLat2CartoCoord(double longitude, double latitute, double& x, double& y)
    {
        x = (longitude + 180.0) / 360.0 * GLOBAL_WIDTH;
        y = (latitute + 90.0) / 180.0 * GLOBAL_HEIGHT;
    }

    void GlobalCoord2CartoCoord(const double vec[], double& cartoX, double& cartoY)
    {
        double lon, lat, height;
        GlobalCoord2LongLat(vec, lon, lat, height);
        LongLat2CartoCoord(lon, lat, cartoX, cartoY);
    }

    void GlobalCoord2CartoCoord(const glm::dvec3& vec, double& cartoX, double& cartoY) {
        double lon, lat, height;
        GlobalCoord2LongLat(vec, lon, lat, height);
        LongLat2CartoCoord(lon, lat, cartoX, cartoY);
    }

    //ͬʱ���ظ߶���Ϣ
    void GlobalCoord2CartoHeightCoord(const double vec[], double& cartoX, double& cartoZ, double& height)
    {
        double lon, lat;
        GlobalCoord2LongLat(vec, lon, lat, height);
        LongLat2CartoCoord(lon, lat, cartoX, cartoZ);
    }

    void CartoCoord2GlobalCoord(double vec[], double cartoX, double cartoZ, double height = 0.0)
    {
        double lon, lat;
        CartoCoord2LongLat(cartoX, cartoZ, lon, lat);
        LongLat2GlobalCoord(lon, lat, height, vec);
    }

    void getGlobalAxis(double& laxis, double& saxis)
    {
        laxis = WGS84_ellipse.a;
        saxis = WGS84_ellipse.b;
    }

    double getLongAxis()
    {
        return WGS84_ellipse.a;
    }

    double getShortAxis()
    {
        return WGS84_ellipse.b;
    }
};
extern CCoordChange g_coord;