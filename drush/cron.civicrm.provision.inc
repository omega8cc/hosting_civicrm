<?php

/**
 * Implements the provision-civicrm-cron command.
 *
 * Runs the site's CiviCRM scheduled jobs through cv, as the default
 * organisation with full rights, as cv's own cron does. The panel's CiviCRM
 * cron queue uses it for Backdrop sites, whose CiviCRM ships no Drush
 * commands, and Drupal 10+ sites, whose Drush 8 command files BOA does not
 * load for the backend; a failure is an error, so the queue reports it.
 */
function drush_provision_civicrm_cron() {
  if (!_provision_civicrm_is_site_context() || !file_exists(d()->site_path . '/civicrm.settings.php')) {
    return drush_set_error('PROVISION_CIVICRM_NOT_INSTALLED', dt('CiviCRM is not installed on this site.'));
  }

  $retval = 0;
  $output = _provision_civicrm_run_cv_command('api3 Job.execute --out=json-strict', $retval);
  if ($retval || !is_array($output) || !empty($output['is_error'])) {
    return drush_set_error('PROVISION_CIVICRM_CRON_FAILED', dt('The CiviCRM scheduled jobs failed (cv exit status @status).', array('@status' => $retval)));
  }

  drush_log(dt('CiviCRM: scheduled jobs done.'), 'ok');
}
