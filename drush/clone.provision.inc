<?php

/**
 * @file
 *
 * CiviCRM support module for the Aegir backend.
 *
 * This file contains the clone operations.
 */

/**
 * Implements drush_hook_COMMAND_validate().
 */
function drush_provision_civicrm_provision_clone_validate($new_name = NULL, $platform = NULL) {
  return _provision_civicrm_target_release_check('clone', $platform);
}
