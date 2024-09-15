library(DBI)
library(RSQLite)
library(MASS)


mydb <- dbConnect(RSQLite::SQLite(), "./test.db")

data("birthwt")
names(birthwt) <- c("baby_low_weight", "mom_age", "mom_weight",
                    "mom_race", "mom_smoke", "mom_ptl", "mom_ht",
                    "mom_ui", "mom_ftv", "baby_weight")

rand_chars <- matrix(sample(letters, nrow(birthwt) * 5, replace = TRUE),
                     ncol = nrow(birthwt), nrow = 5)
rand_chars <- apply(rand_chars, 2, paste, collapse = "")
birthwt$rand_char <- rand_chars


dbWriteTable(mydb, "birthwt", birthwt, overwrite = TRUE)
dbDisconnect(mydb, shutdown = TRUE)
