#ifndef WCTE_DATAQUALITY_H
#define WCTE_DATAQUALITY_H

#include <string>
#include <map>

class WCTE_DataQuality {
public:
    WCTE_DataQuality();

    bool LoadQualityInfo(const std::string& json_filename);
    void SetRunID(int run_id);

    bool IsGoodRun() const;

private:
    int current_run_id_;
    std::map<int, bool> run_quality_; // Run ID -> GoodRun flag
};

#endif
