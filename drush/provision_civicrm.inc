<?php

/**
 * @file
 * Hookup with drush
 * Common functions used by install and verify.
 */

/**
 * Custom helper function findDruAppRoot
 */
function findDruAppRoot($startDir) {
  $dir = realpath($startDir);
  while ($dir !== '/' && !is_dir($dir . '/vendor')) {
    $dir = dirname($dir);
  }
  return is_dir($dir . '/vendor') ? $dir : null;
}

/**
 * Platform web root auto-discovery to avoid confusing
 * typical Drupal 8+ app root with Drupal 8+ web root.
 */
function _provision_civicrm_find_platform_web_root($isroot) {

  $test_root_index = $isroot . '/index.php';
  $test_root_docroot = $isroot . '/docroot/index.php';
  $test_root_html = $isroot . '/html/index.php';
  $test_root_web = $isroot . '/web/index.php';

  if (!provision_file()->exists($test_root_index)->status()) {
    drush_log(dt("Platform web root path needs correction, running auto-discovery.."), 'notice');
    if (provision_file()->exists($test_root_docroot)->status()) {
      $isroot = $isroot . '/docroot';
      drush_log(dt("Platform corrected web root: @webroot", array('@webroot' => $isroot)), 'debug');
    }
    elseif (provision_file()->exists($test_root_html)->status()) {
      $isroot = $isroot . '/html';
      drush_log(dt("Platform corrected web root: @webroot", array('@webroot' => $isroot)), 'debug');
    }
    elseif (provision_file()->exists($test_root_web)->status()) {
      $isroot = $isroot . '/web';
      drush_log(dt("Platform corrected web root: @webroot", array('@webroot' => $isroot)), 'debug');
    }
  }
  else {
    drush_log(dt("Platform web root: @webroot", array('@webroot' => $isroot)), 'debug');
  }
  return $isroot;
}

/**
 * Expose the service type this extension defines to provision.
 *
 * @return
 *   An array with the service type the key, and the default implementation the value.
 */
function provision_civicrm_provision_services() {
  provision_civicrm_provision_register_autoload();
  return array(
    'civicrm' => NULL,
    'civicrm_db' => NULL,
  );
}

/**
 * Wrapper to initialize CiviCRM, depending on the CMS.
 *
 * Returns FALSE if this is not a CiviCRM site (ex: hostmaster).
 */
function provision_civicrm_initialize() {
  if (drush_drupal_major_version() >= 8 && \Drupal::hasService('civicrm')) {
    \Drupal::service('civicrm')->initialize();
    return TRUE;
  }
  elseif (function_exists('civicrm_initialize')) {
    civicrm_initialize();
    return TRUE;
  }

  return FALSE;
}

/**
 * Get the default CiviCRM locale.
 * This is inspired from CRM_Core_Smarty::getLocale().
 */
function _provision_civicrm_get_default_locale() {
  global $tsLocale;

  $config = CRM_Core_Config::singleton();

  if (!empty($config->lcMessages)) {
    return $config->lcMessages;
  }

  return 'en_US';
}

/**
 * Get an array of civicrm related options
 *
 * @param $op
 *   A string describing the task being run, for debugging purposes
 */
function _provision_civicrm_drush_options($op = 'unknown') {
  drush_log(dt("CiviCRM: In @op: Fetching CiviCRM Drush options", array('@op' => $op)));

  $civi_opts = array(
    'civicrm_cron_username' => '',
    'civicrm_cron_password' => '',
    'civicrm_sitekey' => '',
  );

  foreach ($civi_opts as $opt => $val) {
    $civi_opts[$opt] = drush_get_option($opt, NULL);
  }

  return $civi_opts;
}

/**
 * Whether the platform of the current context is Backdrop.
 *
 * Backdrop reports Drupal major version 1 to Drush, so every Drupal version
 * test below takes its Drupal 6/7 branch. Those branches load CiviCRM's Drupal
 * Drush file and write Drupal variables in this process, and neither exists for
 * Backdrop: its CiviCRM ships no Drush file, and the Backdrop layer never loads
 * the site's database layer here. Backdrop sites go through cv instead.
 */
function _provision_civicrm_is_backdrop() {
  $root = d()->root;
  if (!is_string($root) || $root === '') {
    return FALSE;
  }
  if (function_exists('provision_platform_is_backdrop')) {
    return provision_platform_is_backdrop($root);
  }
  return file_exists($root . '/core/includes/bootstrap.inc')
    && file_exists($root . '/core/misc/backdrop.js')
    && !file_exists($root . '/autoload.php');
}

/**
 * The cv option that names the site to boot.
 *
 * Without it cv boots the site named by the CIVICRM_UF_BASEURL of the
 * civicrm.settings.php it finds, and a cloned copy of that file still names
 * the source site until it is regenerated: the regeneration then booted the
 * source, and the clone kept the source's URL and private path.
 */
function _provision_civicrm_cv_site_option() {
  if (empty(d()->uri)) {
    return '';
  }
  $protocol = (d()->ssl_enabled || d()->https_enabled) ? 'https' : 'http';
  return ' --url=' . escapeshellarg($protocol . '://' . d()->uri . '/');
}

/**
 * The shell prefix a cv call runs with: the site folder as the working
 * directory, and a umask that lets the web pool write what cv creates.
 *
 * cv runs as the backend user and makes folders under the site's files (its
 * compiled templates among them); with the backend's 022 umask the web pool
 * could not write them and the site failed until the next permissions fix.
 * 002 gives them the group-writable modes Provision gives the files folders.
 * core:install keeps the backend's own umask: it writes civicrm.settings.php,
 * which holds the credentials, with whatever mode the umask leaves.
 */
function _provision_civicrm_cv_prefix($command = '') {
  $prefix = 'cd ' . escapeshellarg(d()->site_path) . ' && ';
  if (strpos(ltrim($command), 'core:install') !== 0) {
    $prefix .= 'umask 002 && ';
  }
  return $prefix . _provision_civicrm_cv_php_path();
}

/**
 * The PATH a cv call runs with, so that cv runs on this task's PHP.
 *
 * cv starts through "#!/usr/bin/env php": a bare cv runs on the PATH's php,
 * the server's default, while the task runs on the PHP chosen for the site's
 * platform. A codebase whose dependencies need a newer PHP than the server's
 * default then fails every cv call. The running PHP's folder goes first when
 * cv can run on it (PHP 7.3 or newer) and the binary is named php; otherwise
 * cv keeps the PATH's php.
 */
function _provision_civicrm_cv_php_path($php = NULL, $version_id = NULL) {
  if ($php === NULL) {
    $php = defined('PHP_BINARY') ? PHP_BINARY : '';
  }
  if ($version_id === NULL) {
    $version_id = PHP_VERSION_ID;
  }
  if ($php === '' || $version_id < 70300 || basename($php) !== 'php' || !is_executable($php)) {
    return '';
  }
  return 'export PATH=' . escapeshellarg(dirname($php)) . ':"$PATH" && ';
}

/**
 * Keep a Backdrop site's CiviCRM private files in the site's private folder.
 *
 * CiviCRM's settings generator, run from the command line, does not see a
 * Backdrop site's private file path and pins CiviCRM's private area (its logs
 * and compiled templates) to the public files folder, on install and on every
 * regeneration; on Drupal it lands in the private one. Run after each of them,
 * so the file is the same from one regeneration to the next.
 */
function _provision_civicrm_backdrop_private_path() {
  $filename = d()->site_path . '/civicrm.settings.php';
  $settings = @file_get_contents($filename);
  if (!$settings) {
    drush_log(dt('CiviCRM: could not read @file to set its private path.', array('@file' => $filename)), 'warning');
    return FALSE;
  }
  $private = d()->site_path . '/private/files/civicrm';
  $lines = array(
    '#^\$civicrm_paths\[\'civicrm\.private\'\]\[\'path\'\] = .*$#m' => '$civicrm_paths[\'civicrm.private\'][\'path\'] = ' . var_export($private, TRUE) . ';',
    '#^(\s*)define\(\'CIVICRM_TEMPLATE_COMPILEDIR\', .*$#m' => 'define(\'CIVICRM_TEMPLATE_COMPILEDIR\', ' . var_export($private . '/templates_c', TRUE) . ');',
  );
  foreach ($lines as $pattern => $line) {
    $count = 0;
    $settings = preg_replace_callback($pattern, function ($m) use ($line) {
      return (isset($m[1]) ? $m[1] : '') . $line;
    }, $settings, 1, $count);
    if (!$count) {
      drush_log(dt('CiviCRM: no line to set in @file: @pattern', array('@file' => $filename, '@pattern' => $pattern)), 'warning');
      return FALSE;
    }
  }
  provision_file()->chmod($filename, 0640);
  $written = file_put_contents($filename, $settings);
  provision_file()->chmod($filename, 0440);
  if ($written === FALSE) {
    drush_log(dt('CiviCRM: could not write @file to set its private path.', array('@file' => $filename)), 'warning');
    return FALSE;
  }
  drush_log(dt('CiviCRM: private files kept in @path', array('@path' => $private)), 'ok');
  return TRUE;
}

/**
 * Check we are in an Aegir site context, and that CiviCRM is available in the platform.
 *
 * @param Boolean $load_if_missing Recommended in most cases. See do#2364871.
 * @see _provision_civicrm_get_package_path();
 */
function _provision_civicrm_is_site_context($load_if_missing = FALSE) {
  if (d()->type == 'site') {
    $crmpath = _provision_civicrm_get_package_path($load_if_missing);
    return ($crmpath ? TRUE : FALSE);
  }
  elseif (d()->type == 'wpsite') {
    // Requires hosting_wordpress in Aegir.
    if (function_exists('civicrm_initialize')) {
      return TRUE;
    }
  }

  return FALSE;
}

/**
 * Get the path where the CiviCRM module is installed
 * Ex: /var/aegir/platforms/[...]/sites/all/modules/civicrm
 *
 * NB: if CiviCRM was not detected, verify the platform.
 *
 * @param Boolean $load_if_missing Recommended in most cases. See do#2364871.
 */
function _provision_civicrm_get_package_path($load_if_missing = FALSE) {
  // Issue#2942572 For D8 and D9, we only support having CiviCRM in /vendor
  // Aegir package info functions are not CiviCRM-friendly. This is simpler.

  if (drush_drupal_major_version() >= 8) {
    $appRoot = findDruAppRoot(d()->root);
    $crmpath = $appRoot . '/vendor/civicrm/civicrm-core/';
    if (is_dir($crmpath)) {
      drush_log(dt('CiviCRM: civicrm is in @path', array('@path' => $crmpath)));
      return $crmpath;
    }
  }

  $module = _provision_civicrm_get_package_info();

  if ($module) {
    // Get the parent directory of the module, which is in civicrm/drupal/civicrm.module
    $crmpath = dirname(dirname($module['filename']));
    drush_log(dt('CiviCRM: civicrm is in @path', array('@path' => $crmpath)));

    // do#2364871 Workaround a BOA 2.3 issue
    // When civicrm is in an install profile, the civicrm drush module does not get loaded.
    // CiviCRM for Backdrop ships no Drush file; Backdrop sites run through cv only.
    if ($load_if_missing && ! function_exists('_civicrm_get_crmpath') && ! _provision_civicrm_is_backdrop()) {
      drush_log(dt('CiviCRM: loading file: civicrm.drush.inc.'));
      require_once($crmpath . '/drupal/drush/civicrm.drush.inc');
    }

    return $crmpath;
  }

  return FALSE;
}

/**
 * Get the CiviCRM module base information (filename, basename, version, etc)
 */
function _provision_civicrm_get_package_info() {
  $module = NULL;

  // Check if the CiviCRM code base is present in the platform
  // Note: after putting the code there, you need to verify
  // the platform.
  $packages = drush_get_option('packages', array('modules' => NULL));
  $profile = drush_get_option('profile', NULL);

  if (isset($packages['modules']) && isset($packages['modules']['civicrm'])) {
    drush_log(dt('CiviCRM: found civicrm in packages'));
    $module = $packages['modules']['civicrm'];
  }
  elseif (isset($packages['sites-all']) && isset($packages['sites-all']['modules']) && isset($packages['sites-all']['modules']['civicrm'])) {
    $module = $packages['sites-all']['modules']['civicrm'];
    drush_log(dt('CiviCRM: found civicrm in packages of sites-all modules'));
  }
  elseif (isset($packages['base']) && isset($packages['base']['modules']) && isset($packages['base']['modules']['civicrm'])) {
    $module = $packages['base']['modules']['civicrm'];
    drush_log(dt('CiviCRM: found civicrm in packages of base modules'));
  }
  elseif ($profile && isset($packages['profiles'][$profile]['modules']) && isset($packages['profiles'][$profile]['modules']['civicrm'])) {
    $module = $packages['profiles'][$profile]['modules']['civicrm'];
    drush_log(dt('CiviCRM: found civicrm in packages of @profile profile modules', array('@profile' => $profile)));
  }
  else {
    // drush_log(dt('CiviCRM: civicrm not found in packages ' . print_r($packages, 1)));
  }

  return $module;
}

/**
 * CiviCRM verify operations (used by both verify and deploy)
 *
 * $url becomes the base URL in civicrm.settings.php (without it the file's own
 * is kept); $new_keys gives the site a new site key, credential keys, signing
 * keys and deploy id, for a site that is new (a Clone). Without it the existing
 * keys are kept and only missing ones are generated.
 */
function _provision_civicrm_regenerate_settings($url = NULL, $new_keys = FALSE) {
  drush_log(dt("CiviCRM: starting provision_civicrm_regenerate_settings()."));

  // We have to manually update the $civicrm_root in the civicrm.settings.php
  // file, because using cv assumes a functionnal CiviCRM, which will not work
  // if we are cloning from a platform to another (duplicate ClassLoader).
  $civicrm_root = _provision_civicrm_get_package_path();
  $filename = d()->site_path . '/civicrm.settings.php';
  $settings = file_get_contents($filename);

  // Relax permissions while we update the file
  provision_file()->chmod(d()->site_path, 0755)
    ->succeed($name . ' permissions of @path have been changed to @perm.')
    ->fail('Could not change permissions <code>@path</code> to @perm');
  provision_file()->chmod($filename, 0640)
    ->succeed($name . ' permissions of @path have been changed to @perm.')
    ->fail('Could not change permissions <code>@path</code> to @perm');

  if ($settings) {
    $replacements = [];
    $replacements[] = [
      'search_regex' => '/^.*?\$civicrm_root =.*\n?/m',
      'replace_line' => "\$civicrm_root = '$civicrm_root';\n",
    ];
    // This is not great because we assume [civicrm.private] points here
    // but .. it should?
    $replacements[] = [
      'search_regex' => "#'CIVICRM_TEMPLATE_COMPILEDIR', '(.*?/)[^/]+/(web/)?sites/[^/]+/(private/)?files/civicrm/templates_c'#",
      'replace_line' => "'CIVICRM_TEMPLATE_COMPILEDIR', '" . d()->site_path . "/private/files/civicrm/templates_c'",
    ];

    foreach ($replacements as $desc => $line) {
      // Throw a warning if any of our replacements cannot be found.
      if (!preg_match($line['search_regex'], $settings)) {
        drush_log('drush_provision_civicrm_provision_deploy: Failed to replace: '. $line['search_regex'], 'warning');
      }
      else {
        $count = 0;
        $settings = preg_replace($line['search_regex'], $line['replace_line'], $settings, -1, $count);
        if ($count > 0) {
          drush_log('drush_provision_civicrm_provision_deploy: Succeeded to replace: ' . $line['search_regex'], 'success');
        }
      }
    }

    file_put_contents($filename, $settings);
  }
  else {
    drush_log(dt("drush_provision_civicrm_provision_deploy: Failed to read the civicrm.settings.php file (%file). The civicrm_root value will not be updated, and cloning from one platform to another will fail.", ['%file' => $filename]), 'error');
  }

  // Make sure to write the civicrm options
  $civi_opts = _provision_civicrm_drush_options('post verify');
  foreach ($civi_opts as $opt => $val) {
    drush_set_option($opt, $val, 'site');
  }

  // Since we do not have an upgrade path for existing site key (before it
  // was added to the frontend), be nice about an empty sitekey.
  if (d()->civicrm_sitekey) {
    drush_log('CiviCRM: found a site key from the front-end.', 'ok');
    drush_set_option('civicrm_sitekey', d()->civicrm_sitekey);
  }
  else {
    drush_log('CiviCRM: no site key found from the front-end.');
  }

  // @todo FIXME pass the civicrm_sitekey if set in the frontend
  _provision_civicrm_run_cv_command('php:script ' . dirname(__DIR__) . '/cv/regenerate-settings.php ' . escapeshellarg($url) . ($new_keys ? ' new-keys' : ''));

  if (_provision_civicrm_is_backdrop()) {
    _provision_civicrm_backdrop_private_path();
  }

  // Set the file back to strict. The site folder stays at Provision's 0755:
  // later steps of the same task write and remove files in it (the database
  // dump a deploy imported, the drushrc.php the task saves), which a closed
  // folder refuses, failing a Restore, Clone or Migrate.
  provision_file()->chmod($filename, 0440)
    ->succeed($name . ' permissions of @path have been changed to @perm.')
    ->fail('Could not change permissions <code>@path</code> to @perm');

  drush_log(dt("CiviCRM: Generated config civicrm.settings.php file"), 'ok');
  drush_log(dt("CiviCRM: leaving provision_civicrm_regenerate_settings()."));
}

function provision_civicrm_verify_common() {
  drush_log(dt("CiviCRM: in provision_civicrm_verify_common()."));

  // Set file acls on civicrm.settings.php, if necessary
  if (function_exists('provisionacl_set_acl')) {
    $group = d()->client_name;
    $file = d()->site_path . '/' . 'civicrm.settings.php';
    drush_log(dt('CiviCRM: Attempting to set acls for @group on @file', array('@group' => $group, '@file' => $file)));

    // Check the group
    if (!provision_posix_groupname($group)) {
      drush_log(dt('CiviCRM: not setting acls on @file for non-existent group @group', array('@file' => $file, '@group' => $group)), 'warning');
    }
    // Check the file
    else if (! file_exists($file)) {
      drush_log(dt('@file not found, acls not configured', array('@file' => $file)), 'warning');
    }
    else {
      provisionacl_set_acl('group', $group, 'r--', array($file), TRUE);
    }
  }

  drush_log(dt("CiviCRM: leaving provision_civicrm_verify_common()."));
  return TRUE;
}

/**
 * Check whether to set the upgrade mode
 */
function _provision_civicrm_check_upgrade_mode() {
  drush_log(dt("CiviCRM: in _provision_civicrm_check_upgrade_mode()"));
  // Sets the civicrm upgrade mode of the code/db version do not match
  $codeVer = _provision_civicrm_codebase_version();
  $dbVer = _provision_civicrm_database_version();

  if (version_compare($codeVer, $dbVer) != 0) {
    if (! defined('CIVICRM_UPGRADE_ACTIVE')) {
      define('CIVICRM_UPGRADE_ACTIVE', 1);
      drush_log(dt('CiviCRM: upgrade mode enabled.'));
      return TRUE;
    }
  }

  drush_log(dt("CiviCRM: leaving _provision_civicrm_check_upgrade_mode()"));
  return FALSE;
}

/**
 * Helper function to get the CiviCRM version of the code base.
 *
 * Avoiding any CMS or Aegir methods, because:
 * - using pm_parse_version() to parse the version, because it does not
 *   like versions such as '8.x-5.0.beta1'.
 * - Drupal7 has $packages['modules']['civicrm']['info']['version'],
 *   but Drupal8 only has $packages['modules']['civicrm']['version'] (which D7 also has).
 * - Sometimes the platform info is outdated unless we remember to run verify.
 * - CiviCRM has a somewhat stable interface to fetch this info.
 */
function _provision_civicrm_codebase_version() {
  static $codeVer = 0;

  if ($codeVer) {
    return $codeVer;
  }

  $crmPath = _provision_civicrm_get_package_path();

  if ($crmPath) {
    $xmlPath = $crmPath . '/xml/version.xml';
    drush_log(dt('CiviCRM: version xmlPath !path', ['!path' => $xmlPath]), 'ok');
    $codeVer = system('grep version_no ' . escapeshellarg($xmlPath) . ' | sed "s/.*>\(.*\)<.*/\1/"');
  }

  drush_log(dt('CiviCRM: code version is !ver', ['!ver' => $codeVer]), 'ok');
  return $codeVer;
}

/**
 * Helper function to get the CiviCRM version of the DB
 *
 * $reset reads it again, for a caller that must see what an upgrade left.
 */
function _provision_civicrm_database_version($reset = FALSE) {
  static $dbVer = 0;

  if ($dbVer && !$reset) {
    return $dbVer;
  }
  $dbVer = 0;

  $command = _provision_civicrm_cv_prefix() . 'echo "select version from civicrm_domain limit 1" | cv sql' . _provision_civicrm_cv_site_option() . ' | tail -1';
  drush_log(dt("CiviCRM: running: %command", ['%command' => $command]), 'ok');
  $dbVer = system($command, $retval);

  if ($retval) {
    drush_log(dt("CiviCRM: command failed {$retval} (the ConfigAndLog should have more details): %output", ['%output' => $dbVer]), 'warning');
    // A failed read is not a release: the next call reads again.
    $dbVer = 0;
    return NULL;
  }

  drush_log(dt('CiviCRM: db version is !ver', ['!ver' => $dbVer]), 'ok');
  return $dbVer;
}

/**
 * Warn when the site's CiviCRM code is older than its database.
 *
 * The state a copy onto an older CiviCRM leaves (before the copying tasks
 * checked the target, or when the check could not read a release): CiviCRM's
 * upgrade runs only forward, so the site's code does not know its schema.
 * Returns TRUE when it warned.
 */
function _provision_civicrm_warn_older_code() {
  $code = trim((string) _provision_civicrm_codebase_version());
  $db = trim((string) _provision_civicrm_database_version());
  if (!preg_match('/^\d+\.\d+/', $code) || !preg_match('/^\d+\.\d+/', $db)) {
    return FALSE;
  }
  if (version_compare($code, $db, '<')) {
    drush_log(dt('CiviCRM: this site runs CiviCRM @code on a database of CiviCRM @db. CiviCRM upgrades a database forward only and does not support older code: move the site to a platform with CiviCRM @db or newer.', array('@code' => $code, '@db' => $db)), 'warning');
    return TRUE;
  }
  return FALSE;
}

/**
 * Say plainly that CiviCRM's database upgrade failed, and what it left.
 *
 * The task keeps its warning status: the Verify that runs the upgrade follows
 * a Migrate or Clone whose copy already is the site, so failing the task would
 * change nothing but its label. The line names the code release and the
 * release the database reads now. A step that threw leaves the database marked
 * (such as "6.20.alpha1.upgrade"), and cv then refuses to start again without
 * --retry or --skip, so a Verify cannot resume it: the line says so and names
 * the way that can. A failure before any step began leaves the old release,
 * and the next Verify tries the upgrade again.
 */
function _provision_civicrm_warn_failed_upgrade() {
  $code = trim((string) _provision_civicrm_codebase_version());
  $code = $code !== '' ? $code : 'unknown';
  $db = trim((string) _provision_civicrm_database_version(TRUE));
  if (!preg_match('/^\d+\.\d+/', $db)) {
    // cv could not read the database either: the upgrade may never have run.
    drush_log(dt('CiviCRM: the database upgrade did not run: cv could not read the site\'s CiviCRM database (code CiviCRM @code). The cv commands above show what was run; once cv can boot the site, run Verify again.', array('@code' => $code)), 'warning');
    return;
  }
  if (stripos($db, 'upgrade') !== FALSE) {
    drush_log(dt('CiviCRM: the database upgrade stopped inside a step and left the database marked @db; the site runs CiviCRM @code and CiviCRM may not work until the step completes. A Verify cannot resume it: CiviCRM\'s log (its ConfigAndLog folder) names the cause. Once that is fixed, a root operator resumes the step with cv upgrade:db --retry for the site (or skips it with --skip), or the site goes back to its backup from before the upgrade.', array('@code' => $code, '@db' => $db)), 'warning');
    return;
  }
  drush_log(dt('CiviCRM: the database upgrade failed. The site runs CiviCRM @code and its database reads @db, so CiviCRM may not work until the upgrade completes. CiviCRM\'s log (its ConfigAndLog folder) names the cause: fix it, then run Verify again to retry the upgrade.', array('@code' => $code, '@db' => $db)), 'warning');
}

/**
 * The CiviCRM release a platform's code carries, or NULL when none is found.
 *
 * Read from the code's xml/version.xml without loading the platform: the
 * vendor copy of a Drupal 8+ platform (registered at its docroot or at its
 * project folder), else the module folders a Backdrop or Drupal 7 platform
 * keeps CiviCRM in. The profiles are listed with scandir(), not glob(): a
 * platform root may hold glob characters.
 */
function _provision_civicrm_platform_release($root) {
  if (!is_string($root) || $root === '' || !is_dir($root)) {
    return NULL;
  }
  $root = rtrim($root, '/');
  $files = array();
  $appRoot = findDruAppRoot($root);
  if ($appRoot) {
    $files[] = $appRoot . '/vendor/civicrm/civicrm-core/xml/version.xml';
  }
  foreach (array('modules/civicrm', 'modules/contrib/civicrm', 'sites/all/modules/civicrm', 'sites/all/modules/contrib/civicrm') as $rel) {
    $files[] = $root . '/' . $rel . '/xml/version.xml';
  }
  if (is_dir($root . '/profiles')) {
    foreach (scandir($root . '/profiles') as $profile) {
      if ($profile !== '.' && $profile !== '..') {
        $files[] = $root . '/profiles/' . $profile . '/modules/civicrm/xml/version.xml';
      }
    }
  }
  foreach ($files as $file) {
    if (!is_file($file)) {
      continue;
    }
    $xml = file_get_contents($file);
    if (is_string($xml) && preg_match('#<version_no>\s*([^<\s]+)\s*</version_no>#', $xml, $match)) {
      return $match[1];
    }
  }
  return NULL;
}

/**
 * Refuse to copy a CiviCRM site onto a platform with an older CiviCRM.
 *
 * CiviCRM upgrades a database forward only: on code older than its schema the
 * Verify that follows the copy runs the extension upgrade alone, and the site
 * is left on a database its code does not know. The check runs before the
 * task's safety copy, so a refused Migrate or Clone changes nothing. When the
 * site's database release or the target's CiviCRM cannot be read, the task
 * goes on with a warning.
 *
 * Returns TRUE to go on, FALSE after setting the error.
 */
function _provision_civicrm_target_release_check($task, $platform) {
  if (!is_string($platform) || $platform === '' || !_provision_civicrm_is_site_context()) {
    return TRUE;
  }
  if (!is_file(d()->site_path . '/civicrm.settings.php')) {
    return TRUE;
  }
  $target = d($platform);
  $root = isset($target->root) ? $target->root : '';
  $site_release = trim((string) _provision_civicrm_database_version());
  if ($site_release === '' || !preg_match('/^\d+\.\d+/', $site_release)) {
    drush_log(dt('CiviCRM: the site\'s database release could not be read; the @task goes on without checking the target\'s CiviCRM release.', array('@task' => $task)), 'warning');
    return TRUE;
  }
  $target_release = _provision_civicrm_platform_release($root);
  if ($target_release === NULL) {
    drush_log(dt('CiviCRM: no CiviCRM release found in the target platform at @root; the @task goes on without checking it.', array('@root' => $root, '@task' => $task)), 'warning');
    return TRUE;
  }
  if (version_compare($target_release, $site_release, '<')) {
    return drush_set_error('PROVISION_CIVICRM_TARGET_OLDER', dt('CiviCRM: the target platform carries CiviCRM @target, older than this site\'s database (@site). CiviCRM cannot take a database back to older code: choose a platform with CiviCRM @site or newer.', array('@target' => $target_release, '@site' => $site_release)));
  }
  drush_log(dt('CiviCRM: the target platform carries CiviCRM @target, the site\'s database @site.', array('@target' => $target_release, '@site' => $site_release)), 'ok');
  return TRUE;
}

/**
 * Run a command by invoking a shell instead of running it in the
 * same process as Aegir, because it makes error handling more difficult.
 *
 * You must call escapeshellarg() before calling this function, if there are any args in the $command.
 *
 * Errors are logged as warnings to avoid an Aegir rollback (because those errors
 * should not happen, and if they do, we want to be able to debug).
 */
function _provision_civicrm_run_drush_command($command) {
  $command = 'cd ' . escapeshellarg(d()->site_path) . ' && drush --quiet cc drush && drush --quiet @' . escapeshellarg(d()->uri) . ' ' . $command;
  drush_log(dt("CiviCRM: running: %command", ['%command' => $command]), 'ok');
  $json = system($command, $retval);
  // This assumes we are always using --out=json for CiviCRM
  $output = json_decode($json, TRUE);
  if ($retval == 1 && $json == 'Do you really want to continue? (y/n): y') {
    // Not seeing any error.. shrug
    drush_log(dt("CiviCRM: command success: %output", ['%output' => $json]), 'ok');
  }
  elseif ($retval || (is_array($output) && !empty($output['is_error']))) {
    drush_log(dt("CiviCRM: command failed {$retval} (the ConfigAndLog should have more details): %output", ['%output' => $json]), 'warning');
  }
  elseif (!preg_match('/is_error.:0/', $json)) {
    drush_log(dt("CiviCRM: command may have failed: %output", ['%output' => $json]), 'warning');
  }
  else {
    drush_log(dt("CiviCRM: command success: %output", ['%output' => $json]), 'ok');
  }

  return $json;
}

/**
 * Run a command by invoking a shell instead of running it in the
 * same process as Aegir, because it makes error handling more difficult.
 *
 * You must call escapeshellarg() before calling this function, if there are any args in the $command.
 *
 * Errors are logged as warnings to avoid an Aegir rollback (because those errors
 * should not happen, and if they do, we want to be able to debug).
 *
 * $retval receives cv's exit status, for callers that must report a failure.
 */
function _provision_civicrm_run_cv_command($command, &$retval = NULL) {
  // Always non-interactive (cv -n)
  $command = _provision_civicrm_cv_prefix($command) . 'cv -n' . _provision_civicrm_cv_site_option() . ' ' . $command;
  drush_log(dt("CiviCRM: running: %command", ['%command' => $command]), 'ok');
  $json = system($command, $retval);
  // Output is always json
  $output = json_decode($json, TRUE);
  if ($retval || (is_array($output) && !empty($output['is_error']))) {
    drush_log(dt("CiviCRM: command failed {$retval} (the ConfigAndLog should have more details): %output", ['%output' => $json]), 'warning');
  }
  else {
    drush_log(dt("CiviCRM: command success: %output", ['%output' => $json]), 'ok');
  }

  return $output;
}
