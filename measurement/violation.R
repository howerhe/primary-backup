#!/usr/bin/env Rscript
suppressPackageStartupMessages(library(conflicted))
suppressPackageStartupMessages(library(tidyverse))

options(readr.show_col_types = FALSE)

# Read data.
violations = read_csv("violation_summary.csv")

p <- ggplot(violations) +
  geom_tile(aes(x = thread, y = store, fill = violation)) +
  scale_x_continuous(trans='log2') +
  scale_y_continuous(trans='log2')

ggsave("violation_summary.png", plot = p)