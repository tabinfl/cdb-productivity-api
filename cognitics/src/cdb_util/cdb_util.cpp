
#include <cdb_util/cdb_util.h>

#include <cdb_util/FeatureDataDictionary.h>

#include <cdb_tile/TileLatitude.h>
#include <ccl/miniz.h>

#include <dbflib/DBaseFile.h>

#include <ogr/File.h>
#include <flt/OpenFlight.h>
#include <flt/TexturePalette.h>

#include <array>
#include <cctype>
#include <locale>
#include <iomanip>

#if _WIN32
#include <filesystem>
namespace std { namespace filesystem = std::experimental::filesystem; }
#elif __GNUC__ && (__GNUC__ < 8)
#include <experimental/filesystem>
namespace std { namespace filesystem = std::experimental::filesystem; }
#else
#include <filesystem>
#endif

namespace cognitics {
namespace cdb {

int LodForPixelSize(double pixel_size)
{
    for(int i = -10; i <= 23; ++i)
    {
        double lod_pixel_size = (1.0 / 1024) / std::pow(2, i);
        if(lod_pixel_size < pixel_size)
            return i;
    }
    return 23;
}

double PixelSizeForLod(int lod)
{
    auto div = std::max<int>(std::pow(2, lod), 1);
    return 1.0 / div;
}

int TileDimensionForLod(int lod)
{
    return std::min<int>(std::pow(2, lod + 10), 1024);
}

double MinimumPixelSizeForLod(int lod, double latitude)
{
    auto div = std::max<int>(std::pow(2, lod), 1);
    auto lat_spacing = 1.0 / div;
    auto tile_width = (int)get_tile_width(latitude);
    auto lon_spacing = (double)tile_width / div;
    return std::min<double>(lat_spacing, lon_spacing);
}

std::vector<std::string> FileNamesForTiledDataset(const std::string& cdb, int dataset)
{
    auto result = std::vector<std::string>();
    auto tiles_path = cdb + "/Tiles";
    if(!ccl::directoryExists(tiles_path))
        return result;
    for(const auto& lat_entry : ccl::FileInfo::getSubDirectories(tiles_path))
    {
        for(const auto& lon_entry : ccl::FileInfo::getSubDirectories(lat_entry))
        {
            auto dataset_prefix = std::to_string(dataset);
            while(dataset_prefix.length() < 3)
                dataset_prefix = "0" + dataset_prefix;
            for(const auto& dataset_entry : ccl::FileInfo::getSubDirectories(lon_entry))
            {
                auto dataset_dir = ccl::FileInfo(dataset_entry).getBaseName();
                if(dataset_dir.substr(0, 3) != dataset_prefix)
                    continue;
                for(const auto& lod_entry : ccl::FileInfo::getSubDirectories(dataset_entry))
                {
                    for(const auto& uref_entry : ccl::FileInfo::getSubDirectories(lod_entry))
                    {
                        for(const auto& entry : ccl::FileInfo::getAllFiles(uref_entry, "*.*"))
                            result.push_back(entry.getFileName());
                    }
                }
            }
        }
    }
    return result;
}

std::vector<TileInfo> FeatureTileInfoForTiledDataset(const std::string& cdb, int dataset, std::tuple<double, double, double, double> nsew)
{
    auto filenames = cognitics::cdb::FileNamesForTiledDataset(cdb, dataset);
    auto tiles = cognitics::cdb::TileInfoForFileNames(filenames);
    tiles.erase(std::remove_if(tiles.begin(), tiles.end(), [](const TileInfo& tile) { return !((tile.selector2 == 1) || (tile.selector2 == 3) || (tile.selector2 == 5) || (tile.selector2 == 7) || (tile.selector2 == 9)); }), tiles.end());
    if (std::get<0>(nsew) != DBL_MAX)
    {
        tiles.erase(std::remove_if(tiles.begin(), tiles.end(), [=](const TileInfo& tile)
            {
                double tile_north, tile_south, tile_east, tile_west;
                std::tie(tile_north, tile_south, tile_east, tile_west) = cognitics::cdb::NSEWBoundsForTileInfo(tile);
                return (tile_north <= std::get<1>(nsew)) || (tile_south >= std::get<0>(nsew)) || (tile_east <= std::get<3>(nsew)) || (tile_west >= std::get<2>(nsew));
            }
        ), tiles.end());
    }
    return tiles;
}

TileInfo TileInfoForFileName(const std::string& filename)
{
    auto tokens = std::vector<std::string>();
    auto token = std::string();
    auto iss = std::istringstream(filename);
    while (std::getline(iss, token, '_'))
        tokens.push_back(token);
    if (tokens.size() < 7)
        throw std::runtime_error("invalid tile filename");
    auto result = TileInfo();
    auto ns = tokens[0][0];
    result.latitude = std::stoi(tokens[0].substr(1));
    if (ns == 'S')
        result.latitude *= -1;
    auto ew = tokens[0][3];
    result.longitude = std::stoi(tokens[0].substr(4));
    if (ew == 'W')
        result.longitude *= -1;
    result.dataset = std::stoi(tokens[1].substr(1));
    result.selector1 = std::stoi(tokens[2].substr(1));
    result.selector2 = std::stoi(tokens[3].substr(1));
    if(tokens[4][1] == 'C')
        result.lod = -std::stoi(tokens[4].substr(2));
    else
        result.lod = std::stoi(tokens[4].substr(1));
    result.uref = std::stoi(tokens[5].substr(1));
    result.rref = std::stoi(tokens[6].substr(1));
    return result;
}

std::string FilePathForTileInfo(const TileInfo& tileinfo)
{
    std::stringstream ss;
    char ns = (tileinfo.latitude >= 0) ? 'N' : 'S';
    int latitude = std::abs(tileinfo.latitude);
    char ew = (tileinfo.longitude >= 0) ? 'E' : 'W';
    int longitude = std::abs(tileinfo.longitude);
    ss << ns << std::setfill('0') << std::setw(2) << latitude;
    ss << "/" << ew << std::setfill('0') << std::setw(3) << longitude;
    ss << "/" << Dataset(tileinfo.dataset).subdir();
    if(tileinfo.lod < 0)
        return ss.str() + "/LC/U0";
    ss << "/L" << std::setfill('0') << std::setw(2) << tileinfo.lod;
    ss << "/U" << tileinfo.uref;
    return ss.str();
}

std::string FileNameForTileInfo(const TileInfo& tileinfo)
{
    std::stringstream ss;
    char ns = (tileinfo.latitude >= 0) ? 'N' : 'S';
    int latitude = std::abs(tileinfo.latitude);
    char ew = (tileinfo.longitude >= 0) ? 'E' : 'W';
    int longitude = std::abs(tileinfo.longitude);
    std::string lc = (tileinfo.lod >= 0) ? "L" : "LC";
    int lod = std::abs(tileinfo.lod);
    ss << ns << std::setfill('0') << std::setw(2) << latitude;
    ss << ew << std::setfill('0') << std::setw(3) << longitude;
    ss << "_D" << std::setfill('0') << std::setw(3) << tileinfo.dataset;
    ss << "_S" << std::setfill('0') << std::setw(3) << tileinfo.selector1;
    ss << "_T" << std::setfill('0') << std::setw(3) << tileinfo.selector2;
    ss << "_" << lc << std::setfill('0') << std::setw(2) << lod;
    ss << "_U" << tileinfo.uref;
    ss << "_R" << tileinfo.rref;
    return ss.str();
}

TileInfo TileInfoForTile(const Tile& tile)
{
    auto info = TileInfo();
    info.latitude = std::floor(tile.getCoordinates().low().latitude().value());
    info.longitude = std::floor(tile.getCoordinates().low().longitude().value());
    info.dataset = tile.getDataset().code();
    info.lod = tile.getLod();
    info.selector1 = tile.getCs1();
    info.selector2 = tile.getCs2();
    info.rref = tile.getRref();
    info.uref = tile.getUref();
    return info;
}

std::tuple<double, double, double, double> NSEWBoundsForTileInfo(const TileInfo& tileinfo)
{
    auto div = std::max<int>(std::pow(2, tileinfo.lod), 1);
    auto lat_spacing = 1.0 / div;
    auto tile_width = (int)get_tile_width(double(tileinfo.latitude));
    auto lon_spacing = (double)tile_width / div;
    double south = tileinfo.latitude + (lat_spacing * tileinfo.uref);
    double north = south + lat_spacing;
    double west = tileinfo.longitude + (lon_spacing * tileinfo.rref);
    double east = west + lon_spacing;
    return std::make_tuple(north, south, east, west);
}

std::vector<TileInfo> TileInfoForFileNames(const std::vector<std::string>& filenames)
{
    auto files = std::vector<std::string>();
    std::transform(filenames.begin(), filenames.end(), std::back_inserter(files), [](const std::string& fn) { return ccl::FileInfo(fn).getBaseName(); });
    std::sort(files.begin(), files.end());
    files.erase(std::unique(files.begin(), files.end()), files.end());
    auto result = std::vector<TileInfo>();
    std::transform(files.begin(), files.end(), std::back_inserter(result), [](const std::string& fn) { return TileInfoForFileName(fn); });
    return result;
}

std::vector<sfa::Feature*> FeaturesForOGRFile(const std::string& filename, std::tuple<double, double, double, double> nsew)
{
    auto result = std::vector<sfa::Feature*>();
    if(!ccl::fileExists(filename))
        return result;
    auto file = ogr::File();
    if(!file.open(filename))
        return result;
    auto layers = file.getLayers();
    for(auto layer : layers)
    {
        if(std::get<0>(nsew) != DBL_MAX)
            layer->setSpatialFilter(std::get<3>(nsew), std::get<1>(nsew), std::get<2>(nsew), std::get<0>(nsew));
        layer->resetReading();
        while (sfa::Feature *feature = layer->getNextFeature())
            result.push_back(feature);
    }
    file.close();
    return result;
}

std::vector<ccl::AttributeContainer> AttributesForDBF(const std::string& filename)
{
    auto result = std::vector<ccl::AttributeContainer>();
    try
    {
        if(!ccl::fileExists(filename))
            return result;
        auto dbf = DBaseFile();
        if(!dbf.openFile(filename))
            return result;
        for(DBaseRecord& record : dbf.m_records)
        {
            auto attributes = ccl::AttributeContainer();
            if(record.m_recordData.size() != dbf.m_colDef.size())
                continue;
            for(size_t i = 0, c = dbf.m_colDef.size(); i < c; ++i)
            {
                auto key = dbf.m_colDef[i].m_fieldName;
                auto value = *record.m_recordData[i];
                value.erase(value.begin(), std::find_if(value.begin(), value.end(), [](int ch) { return !std::isspace(ch); }));
                value.erase(std::find_if(value.rbegin(), value.rend(), [](int ch) { return !std::isspace(ch); }).base(), value.end());
                attributes.setAttribute(key, value);
            }
            result.push_back(attributes);
        }
    }
    catch(...)
    {
        std::cerr << "  bad dbf: " << filename << std::endl;
    }
    return result;
}

std::map<std::string, ccl::AttributeContainer> AttributesByCNAM(const std::vector<ccl::AttributeContainer>& attrvec)
{
    auto result = std::map<std::string, ccl::AttributeContainer>();
    for(auto attr : attrvec)
    {
        if(attr.hasAttribute("CNAM"))
            result[attr.getAttributeAsString("CNAM")] = attr;
    }
    return result;
}

std::pair<bool, std::vector<std::string>> TextureFileNamesForModel(const std::string& filename)
{
    auto result = std::vector<std::string>();

    auto flt = (flt::OpenFlight*)nullptr;

    auto parent_path = ccl::FileInfo(filename).getDirName();
    if(ccl::FileInfo(parent_path).getSuffix() == "zip")
    {
        auto model_filename = ccl::FileInfo(filename).getBaseName();
        auto flt_size = size_t(0);
        auto flt_data = mz_zip_extract_archive_file_to_heap(parent_path.c_str(), model_filename.c_str(), &flt_size, 0);
        if(!flt_data)
            return std::make_pair(false, result);
        auto flt_str = std::string((char*)flt_data, flt_size);
        auto flt_ss = std::stringstream(flt_str, std::stringstream::binary | std::stringstream::in);
        flt = flt::OpenFlight::open(flt_ss);
        if (!flt)
            return std::make_pair(false, result);
        while (auto record = flt->getNextRecord())
        {
            if (record->getRecordType() != flt::Record::FLT_TEXTUREPALETTE)
                continue;
            result.push_back(dynamic_cast<flt::TexturePalette*>(record)->fileName);
        }
        flt::OpenFlight::destroy(flt);
        return std::make_pair(true, result);
    }
    flt = flt::OpenFlight::open(filename);
    if (!flt)
        return std::make_pair(false, result);
    while (auto record = flt->getNextRecord())
    {
        if (record->getRecordType() != flt::Record::FLT_TEXTUREPALETTE)
            continue;
        result.push_back(dynamic_cast<flt::TexturePalette*>(record)->fileName);
    }
    flt::OpenFlight::destroy(flt);
    return std::make_pair(true, result);
}

std::vector<std::string> GSModelReferencesForTile(const std::string& cdb, const TileInfo& tileinfo, std::tuple<double, double, double, double> nsew)
{
    auto result = std::vector<std::string>();
    auto shp_filepath = cognitics::cdb::FilePathForTileInfo(tileinfo);
    auto shp_filename = cognitics::cdb::FileNameForTileInfo(tileinfo);
    auto shp = cdb + "/Tiles/" + shp_filepath + "/" + shp_filename + ".shp";
    auto features = cognitics::cdb::FeaturesForOGRFile(shp, nsew);
    if(features.empty())
        return result;

    auto dbf_tile = tileinfo;
    ++dbf_tile.selector2;
    auto dbf_filepath = cognitics::cdb::FilePathForTileInfo(dbf_tile);
    auto dbf_filename = cognitics::cdb::FileNameForTileInfo(dbf_tile);
    auto dbf = cdb + "/Tiles/" + dbf_filepath + "/" + dbf_filename + ".dbf";

    auto attrvec = cognitics::cdb::AttributesForDBF(dbf);
    auto attrmap = cognitics::cdb::AttributesByCNAM(attrvec);
    if(attrmap.empty())
        return result;

    auto zip_tile = tileinfo;
    zip_tile.dataset = 300;
    zip_tile.selector1 = 1;
    zip_tile.selector2 = 1;
    auto zip_filepath = cognitics::cdb::FilePathForTileInfo(zip_tile);
    auto zip_filename = cognitics::cdb::FileNameForTileInfo(zip_tile);
    auto zip = cdb + "/Tiles/" + zip_filepath + "/" + zip_filename + ".zip";

    for (auto feature : features)
    {
        auto cnam = feature->attributes.getAttributeAsString("CNAM");
        if(attrmap.find(cnam) == attrmap.end())
            continue;
        auto facc = attrmap[cnam].getAttributeAsString("FACC");
        auto fsc = attrmap[cnam].getAttributeAsString("FSC");
        while (fsc.size() < 3)
            fsc = "0" + fsc;
        auto modl = attrmap[cnam].getAttributeAsString("MODL");
        auto model_filename = zip + "/" + zip_filename + "_" + facc + "_" + fsc + "_" + modl + ".flt";
        result.push_back(model_filename);
    }

    return result;
}

std::vector<std::string> GTModelReferencesForTile(const std::string& cdb, const TileInfo& tileinfo, std::tuple<double, double, double, double> nsew)
{
    auto result = std::vector<std::string>();
    auto shp_filepath = cognitics::cdb::FilePathForTileInfo(tileinfo);
    auto shp_filename = cognitics::cdb::FileNameForTileInfo(tileinfo);
    auto shp = cdb + "/Tiles/" + shp_filepath + "/" + shp_filename + ".shp";
    auto features = cognitics::cdb::FeaturesForOGRFile(shp, nsew);
    if(features.empty())
        return result;

    auto dbf_tile = tileinfo;
    ++dbf_tile.selector2;
    auto dbf_filepath = cognitics::cdb::FilePathForTileInfo(dbf_tile);
    auto dbf_filename = cognitics::cdb::FileNameForTileInfo(dbf_tile);
    auto dbf = cdb + "/Tiles/" + dbf_filepath + "/" + dbf_filename + ".dbf";

    auto attrvec = cognitics::cdb::AttributesForDBF(dbf);
    auto attrmap = cognitics::cdb::AttributesByCNAM(attrvec);
    if(attrmap.empty())
        return result;

    auto fdd = cognitics::cdb::FeatureDataDictionary();

    for (auto feature : features)
    {
        auto cnam = feature->attributes.getAttributeAsString("CNAM");
        if(attrmap.find(cnam) == attrmap.end())
            continue;
        auto facc = attrmap[cnam].getAttributeAsString("FACC");
        auto fsc = attrmap[cnam].getAttributeAsString("FSC");
        while (fsc.size() < 3)
            fsc = "0" + fsc;
        auto modl = attrmap[cnam].getAttributeAsString("MODL");
        auto model_filename = cdb + "/GTModel/500_GTModelGeometry/" + fdd.Subdirectory(facc) + "/D500_S001_T001_" + facc + "_" + fsc + "_" + modl + ".flt";
        result.push_back(model_filename);
    }

    return result;
}


bool TextureExists(const std::string& filename)
{
    //you can iterate through all the files in an archive(using mz_zip_reader_get_num_files()) and retrieve detailed info on each file by calling mz_zip_reader_file_stat().

    auto parent_path = ccl::FileInfo(filename).getDirName();
    if(ccl::FileInfo(parent_path).getSuffix() == "zip")
    {
        auto texture_filename = ccl::FileInfo(filename).getBaseName();
        mz_zip_archive zip;
        memset(&zip, 0, sizeof(zip));
        //auto z = "D:/CDB/LosAngeles_CDB/Tiles/N34/W118/301_GSModelTexture/L06/U0/N34W118_D301_S001_T001_L06_U0_R1.zip";
        //if(!mz_zip_reader_init_file(&zip, z, 0))
        if(!mz_zip_reader_init_file(&zip, parent_path.c_str(), 0))
            return false;
        bool result = (mz_zip_reader_locate_file(&zip, texture_filename.c_str(), nullptr, 0) >= 0);
        mz_zip_reader_end(&zip);
        return result;
    }

    return ccl::FileInfo::fileExists(filename);
}

bool BuildImageryTileBytesFromSampler(GDALRasterSampler& sampler, const TileInfo& tileinfo, std::vector<unsigned char>& bytes)
{
    auto extents = gdalsampler::GeoExtents();
    std::tie(extents.north, extents.south, extents.east, extents.west) = NSEWBoundsForTileInfo(tileinfo);
    extents.width = TileDimensionForLod(tileinfo.lod);
    extents.height = extents.width;
    return sampler.Sample(extents, &bytes[0]);
}

RasterInfo ReadRasterInfo(const std::string& filename)
{
    auto info = RasterInfo();
    auto ds = (GDALDataset*)GDALOpen(filename.c_str(), GA_ReadOnly);
    if(ds == nullptr)
        return info;

    info.Width = ds->GetRasterXSize();
    info.Height = ds->GetRasterYSize();
    auto geotransform = std::array<double, 6>();
    auto has_geotransform = (ds->GetGeoTransform(&geotransform[0]) == CE_None);
    auto projref = ds->GetProjectionRef();
    GDALClose(ds);

    if(!has_geotransform)
        return info;

    auto file_srs = OGRSpatialReference(projref);
    auto app_srs = OGRSpatialReference();
    app_srs.SetWellKnownGeogCS("WGS84");
    auto transform = OGRCreateCoordinateTransformation(&file_srs, &app_srs);

    info.OriginX = geotransform[0];
    info.OriginY = geotransform[3];
    info.PixelSizeX = geotransform[1];
    info.PixelSizeY = geotransform[5];

    auto x_min = info.OriginX;
    auto y_min = info.OriginY;
    auto x_max = info.OriginX + (info.Width * info.PixelSizeX);
    auto y_max = info.OriginY + (info.Height * info.PixelSizeY);

    info.South = (y_max > y_min) ? y_min : y_max;
    info.North = (y_max > y_min) ? y_max : y_min;
    info.West = (x_max > x_min) ? x_min : x_max;
    info.East = (x_max > x_min) ? x_max : x_min;

    if(transform)
    {
        transform->Transform(1, &info.West, &info.South);
        transform->Transform(1, &info.East, &info.North);
        OGRCoordinateTransformation::DestroyCT(transform);
    }

    return info;
}

std::vector<unsigned char> BytesFromJP2(const std::string& filename)
{
    auto result = std::vector<unsigned char>();

    auto dataset = (GDALDataset*)GDALOpen(filename.c_str(), GA_ReadOnly);
    if(!dataset)
        return result;

    auto width = dataset->GetRasterXSize();
    auto height = dataset->GetRasterYSize();
    auto depth = dataset->GetRasterCount();

    double geotransform[6];
    if(dataset->GetGeoTransform(geotransform) != CE_None)
    {
        GDALClose(dataset);
        return result;
    }

    result.resize(width * height * depth);
    auto discard1 = dataset->GetRasterBand(1)->RasterIO(GF_Read, 0, 0, width, height, (unsigned char*)&result[0], width, height, GDT_Byte, 3, width * 3);
    auto discard2 = dataset->GetRasterBand(2)->RasterIO(GF_Read, 0, 0, width, height, (unsigned char*)&result[1], width, height, GDT_Byte, 3, width * 3);
    auto discard3 = dataset->GetRasterBand(3)->RasterIO(GF_Read, 0, 0, width, height, (unsigned char*)&result[2], width, height, GDT_Byte, 3, width * 3);

    GDALClose(dataset);

    return result;
}


bool WriteBytesToJP2(const std::string& filename, const RasterInfo& rasterinfo, const std::vector<unsigned char>& bytes)
{
    auto mem = GetGDALDriverManager()->GetDriverByName("MEM");
    if(mem == NULL)
        return false;
    auto jp2 = GetGDALDriverManager()->GetDriverByName("JP2OpenJPEG");
    if(jp2 == NULL)
        return false;

    double geotransform[6] = { rasterinfo.OriginX, rasterinfo.PixelSizeX, 0.0, rasterinfo.OriginY, 0.0, rasterinfo.PixelSizeY };

    auto mem_ds = mem->Create("mem.tmp", rasterinfo.Width, rasterinfo.Height, 3, GDT_Byte, nullptr);
    mem_ds->SetGeoTransform(geotransform);

    OGRSpatialReference oSRS;
    oSRS.SetWellKnownGeogCS("WGS84");
    char *wkt = NULL;
    oSRS.exportToWkt(&wkt);
    mem_ds->SetProjection(wkt);
    CPLFree(wkt);

    auto discard1 = mem_ds->GetRasterBand(1)->RasterIO(GF_Write, 0, 0, rasterinfo.Width, rasterinfo.Height, (unsigned char*)&bytes[0], rasterinfo.Width, rasterinfo.Height, GDT_Byte, 3, rasterinfo.Width * 3);
    auto discard2 = mem_ds->GetRasterBand(2)->RasterIO(GF_Write, 0, 0, rasterinfo.Width, rasterinfo.Height, (unsigned char*)&bytes[1], rasterinfo.Width, rasterinfo.Height, GDT_Byte, 3, rasterinfo.Width * 3);
    auto discard3 = mem_ds->GetRasterBand(3)->RasterIO(GF_Write, 0, 0, rasterinfo.Width, rasterinfo.Height, (unsigned char*)&bytes[2], rasterinfo.Width, rasterinfo.Height, GDT_Byte, 3, rasterinfo.Width * 3);

    auto out_ds = jp2->CreateCopy(filename.c_str(), mem_ds, 1, NULL, NULL, NULL);
    GDALClose(out_ds);

    GDALClose(mem_ds);

    return true;
}

RasterInfo RasterInfoFromTileInfo(const cognitics::cdb::TileInfo& tileinfo)
{
    auto result = RasterInfo();
    std::tie(result.North, result.South, result.East, result.West) = cognitics::cdb::NSEWBoundsForTileInfo(tileinfo);
    result.OriginX = result.West;
    result.OriginY = result.North;
    result.Width = cognitics::cdb::TileDimensionForLod(tileinfo.lod);
    result.Height = result.Width;
    result.PixelSizeX = (result.East - result.West) / result.Width;
    result.PixelSizeY = (result.South - result.North) / result.Height;
    return result;
}

std::vector<unsigned char> FlippedVertically(const std::vector<unsigned char>& bytes, size_t width, size_t height, size_t depth)
{
    auto result = std::vector<unsigned char>(bytes.size());
    size_t row_size = width * depth;
    for(size_t y = 0; y < height; ++y)
    {
        size_t source_begin = (height - y - 1) * row_size;
        size_t source_end = source_begin + row_size - 1;
        size_t result_begin = y * row_size;
        std::copy(bytes.begin() + source_begin, bytes.begin() + source_end, result.begin() + result_begin);
    }
    return result;
}

bool BuildImageryTileFromSampler(const std::string& cdb, GDALRasterSampler& sampler, const TileInfo& tileinfo)
{
    auto jp2_filepath = cognitics::cdb::FilePathForTileInfo(tileinfo);
    auto jp2_filename = cognitics::cdb::FileNameForTileInfo(tileinfo);
    auto outfilename = cdb + "/Tiles/" + jp2_filepath + "/" + jp2_filename + ".jp2";

    auto bytes = std::vector<unsigned char>();
    if(std::filesystem::exists(outfilename))
        bytes = BytesFromJP2(outfilename);
    if(bytes.empty())
    {
        auto dimension = TileDimensionForLod(tileinfo.lod);
        bytes.resize(dimension * dimension * 3);
    }

    cognitics::cdb::BuildImageryTileBytesFromSampler(sampler, tileinfo, bytes);
    auto dim = cognitics::cdb::TileDimensionForLod(tileinfo.lod);
    bytes = cognitics::cdb::FlippedVertically(bytes, dim, dim, 3);
    auto info = RasterInfoFromTileInfo(tileinfo);
    ccl::makeDirectory(ccl::FileInfo(outfilename).getDirName());
    std::remove(outfilename.c_str());
    return cognitics::cdb::WriteBytesToJP2(outfilename, info, bytes);
}

namespace
{
    ccl::ObjLog log;
    int CPL_STDCALL GDALProgressObserver(CPL_UNUSED double dfComplete, CPL_UNUSED const char *pszMessage, void * /* pProgressArg */)
    {
        log << (dfComplete * 100.0f) << "% complete..." << log.endl;
        return TRUE;
    }
}

bool BuildImageryOverviews(const std::string& cdb)
{
    CPLSetConfigOption("LODMIN", "-10");
    //CPLSetConfigOption("LODMAX", argv[3]);

    //std::string cdbElevationOpenString = "CDB:" + rootCDBOutput + ":Elevation_PrimaryTerrainElevation";
    //const char *gdalErrMsg = CPLGetLastErrorMsg();

    auto open = "CDB:" + cdb + ":Imagery_Yearly";
    auto dataset = (GDALDataset *)GDALOpen(open.c_str(), GA_Update);
    if (dataset == NULL)
        return false;
    if (dataset->BuildOverviews("average", 0, NULL, 0, NULL, GDALProgressObserver, NULL) != CE_None)
        return false;
    GDALClose(dataset);
    return true;
}

bool IsCDB(const std::string& cdb)
{
    return ccl::fileExists(cdb + "/Metadata/Version.xml");
}

bool MakeCDB(const std::string& cdb)
{
    if(IsCDB(cdb))
        return false;
    if(!ccl::makeDirectory(cdb + "/Metadata"))
        return false;
    auto filename = cdb + "/Metadata/Version.xml";
    std::ofstream outfile(filename.c_str());
    if(!outfile.good())
        return false;
    outfile << "<?xml version = \"1.0\"?>\n";
    outfile << "<Version xmlns:xsi = \"http://www.w3.org/2001/XMLSchema-instance\" xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\">\n";
    outfile << "<PreviousIncrementalRootDirectory name = \"\" />\n";
    outfile << "<Comment>Created by mesh2cdb</Comment>\n";
    outfile << "</Version>\n";
    outfile.close();
    return true;
}

}
}
