#include <memory>

#include <gtest/gtest.h>

#include "data_format.h"
#include "data_format_parser.h"

class DataFormatTests : public ::testing::Test {};

TEST_F(DataFormatTests, GoodFormatParseFromFile)
{
    DataFormatParser p("data_format.xsd");
    EXPECT_NO_THROW(p.parseFromFile("good_format.xml"));
}

TEST_F(DataFormatTests, GoodFormatParseFromString)
{
    DataFormatParser p("data_format.xsd");
    EXPECT_NO_THROW(p.parseFromString("<?xml version=\"1.0\" encoding=\"UTF-8\"?><format version=\"BEERR01\">"
                                      "<item name=\"column1\" type=\"string\" size=\"255\" offset=\"0\"/>"
                                      "<item name=\"column2\" type=\"integer\" size=\"4\" offset=\"255\"/></format>"));
}

TEST_F(DataFormatTests, VersionStringTooLong)
{
    DataFormatParser p("data_format.xsd");
    EXPECT_ANY_THROW(p.parseFromString("<?xml version=\"1.0\" encoding=\"UTF-8\"?><format version=\"123456789\">"
                                       "<item name=\"column1\" type=\"string\" size=\"255\" offset=\"0\"/>"
                                       "<item name=\"column2\" type=\"integer\" size=\"4\" offset=\"255\"/></format>"));
}

TEST_F(DataFormatTests, NegativeValues)
{
    DataFormatParser p("data_format.xsd");
    EXPECT_ANY_THROW(p.parseFromString("<?xml version=\"1.0\" encoding=\"UTF-8\"?><format version=\"123456789\">"
                                       "<item name=\"column1\" type=\"string\" size=\"255\" offset=\"-1\"/>"
                                       "<item name=\"column2\" type=\"integer\" size=\"4\" offset=\"255\"/></format>"));
    EXPECT_ANY_THROW(p.parseFromString("<?xml version=\"1.0\" encoding=\"UTF-8\"?><format version=\"123456789\">"
                                       "<item name=\"column1\" type=\"string\" size=\"-1\" offset=\"0\"/>"
                                       "<item name=\"column2\" type=\"integer\" size=\"4\" offset=\"255\"/></format>"));
}

TEST_F(DataFormatTests, BadFormatParseFromFile)
{
    DataFormatParser a("data_format.xsd");
    DataFormatParser b("bad_schema.xsd");
    DataFormatParser c("data_format.xsd");
    DataFormatParser d("bad_schema.xsd");

    EXPECT_ANY_THROW(a.parseFromFile("missing_items_format.xml"));
    EXPECT_ANY_THROW(b.parseFromFile("good_format.xml"));
    EXPECT_ANY_THROW(c.parseFromFile("missing_name_format.xml"));
    EXPECT_ANY_THROW(d.parseFromFile("missing_name_format.xml"));
}

TEST_F(DataFormatTests, BadFormatParseFromString)
{
    DataFormatParser a("data_format.xsd");
    EXPECT_ANY_THROW(a.parseFromString("<?xml version=\"1.0\" encoding=\"UTF-8\"?><form"));
}

TEST_F(DataFormatTests, InvalidSchemaFile)
{
    EXPECT_ANY_THROW(DataFormatParser p("this_file_doesnt_exist.xsd"));
}

TEST_F(DataFormatTests, InvalidXmlFile)
{
    DataFormatParser p("data_format.xsd");
    EXPECT_ANY_THROW(p.parseFromFile("this_file_doesnt_exist.xml"));
}

TEST_F(DataFormatTests, Print)
{
    DataFormatParser p("data_format.xsd");
    std::shared_ptr<DataFormat> df = p.parseFromFile("good_format.xml");
    std::stringstream ss;
    ss << *df.get();
    EXPECT_STREQ(ss.str().c_str(), "version: BEERR01\ncolumn1 string\ncolumn2 integer\n");
}

TEST_F(DataFormatTests, SchemaCheckerGood)
{
    DataFormatParser p("data_format.xsd");
    std::vector<AbstractDataRefItem*> dataRefItems;
    uint32_t int1;
    uint32_t int2;
    dataRefItems.push_back(new DataRefItem<uint32_t>("int1", &int1));
    dataRefItems.push_back(new DataRefItem<uint32_t>("int2", &int2));

    EXPECT_NO_THROW(p.createFromDataRefItems(dataRefItems, "test"));

    EXPECT_TRUE(p.isValid(p.getXmlStr(), dataRefItems.size()));
}
