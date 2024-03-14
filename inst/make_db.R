
library(DBI)
library(RSQLite)
library(MASS)


mydb <- dbConnect(RSQLite::SQLite(), "./test.db")


data("birthwt")
names(birthwt) <- c("baby_low_weight", "mom_age", "mom_weight",
                    "mom_race", "mom_smoke", "mom_ptl", "mom_ht",
                    "mom_ui", "mom_ftv", "baby_weight")

dbWriteTable(mydb, "birthwt", birthwt, overwrite = TRUE)
