<?php

/**
 * Implements the provision-civicrm-enable command.
 */
function drush_provision_civicrm_enable() {
  drush_log('WordPress: starting drush_provision_civicrm_enable', 'ok');

  d()->site_enabled = TRUE;
  d()->redirection = FALSE;
  d()->write_alias();

  d()->service('http')->create_config('site');
  d()->service('http')->parse_configs();

  drush_log('WordPress: finished drush_provision_civicrm_enable', 'ok');
}
