<?php

// Fix missing log tables or columns
// Useful when upgrades have failed
$logging = new CRM_Logging_Schema();
$logging->fixSchemaDifferences();
